/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * 
 * WGL_ARB_pixel_format extension implementation.
 * 
 * @sa http://www.opengl.org/registry/specs/ARB/wgl_pixel_format.txt
 */


#include <windows.h>

#define WGL_WGLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/wglext.h>

#include "pipe/p_compiler.h"
#include "util/u_memory.h"
#include "stw_device.h"
#include "stw_pixelformat.h"


static boolean
stw_query_attrib(
   int iPixelFormat,
   int iLayerPlane,
   int attrib,
   int *pvalue )
{
   uint count;
   uint index;
   const struct stw_pixelformat_info *pfi;

   count = stw_pixelformat_get_extended_count();

   if (attrib == WGL_NUMBER_PIXEL_FORMATS_ARB) {
      *pvalue = (int) count;
      return TRUE;
   }

   index = (uint) iPixelFormat - 1;
   if (index >= count)
      return FALSE;

   pfi = stw_pixelformat_get_info( index );

   switch (attrib) {
   case WGL_DRAW_TO_WINDOW_ARB:
      *pvalue = pfi->pfd.dwFlags & PFD_DRAW_TO_WINDOW ? TRUE : FALSE;
      return TRUE;

   case WGL_DRAW_TO_BITMAP_ARB:
      *pvalue = pfi->pfd.dwFlags & PFD_DRAW_TO_BITMAP ? TRUE : FALSE;
      return TRUE;

   case WGL_NEED_PALETTE_ARB:
      *pvalue = pfi->pfd.dwFlags & PFD_NEED_PALETTE ? TRUE : FALSE;
      return TRUE;

   case WGL_NEED_SYSTEM_PALETTE_ARB:
      *pvalue = pfi->pfd.dwFlags & PFD_NEED_SYSTEM_PALETTE ? TRUE : FALSE;
      return TRUE;

   case WGL_SWAP_METHOD_ARB:
      *pvalue = pfi->pfd.dwFlags & PFD_SWAP_COPY ? WGL_SWAP_COPY_ARB : WGL_SWAP_UNDEFINED_ARB;
      return TRUE;

   case WGL_SWAP_LAYER_BUFFERS_ARB:
      *pvalue = FALSE;
      return TRUE;

   case WGL_NUMBER_OVERLAYS_ARB:
      *pvalue = 0;
      return TRUE;

   case WGL_NUMBER_UNDERLAYS_ARB:
      *pvalue = 0;
      return TRUE;
   }

   if (iLayerPlane != 0)
      return FALSE;

   switch (attrib) {
   case WGL_ACCELERATION_ARB:
      *pvalue = WGL_FULL_ACCELERATION_ARB;
      break;

   case WGL_TRANSPARENT_ARB:
      *pvalue = FALSE;
      break;

   case WGL_TRANSPARENT_RED_VALUE_ARB:
   case WGL_TRANSPARENT_GREEN_VALUE_ARB:
   case WGL_TRANSPARENT_BLUE_VALUE_ARB:
   case WGL_TRANSPARENT_ALPHA_VALUE_ARB:
   case WGL_TRANSPARENT_INDEX_VALUE_ARB:
      break;

   case WGL_SHARE_DEPTH_ARB:
   case WGL_SHARE_STENCIL_ARB:
   case WGL_SHARE_ACCUM_ARB:
      *pvalue = TRUE;
      break;

   case WGL_SUPPORT_GDI_ARB:
      *pvalue = pfi->pfd.dwFlags & PFD_SUPPORT_GDI ? TRUE : FALSE;
      break;

   case WGL_SUPPORT_OPENGL_ARB:
      *pvalue = pfi->pfd.dwFlags & PFD_SUPPORT_OPENGL ? TRUE : FALSE;
      break;

   case WGL_DOUBLE_BUFFER_ARB:
      *pvalue = pfi->pfd.dwFlags & PFD_DOUBLEBUFFER ? TRUE : FALSE;
      break;

   case WGL_STEREO_ARB:
      *pvalue = pfi->pfd.dwFlags & PFD_STEREO ? TRUE : FALSE;
      break;

   case WGL_PIXEL_TYPE_ARB:
      switch (pfi->pfd.iPixelType) {
      case PFD_TYPE_RGBA:
         *pvalue = WGL_TYPE_RGBA_ARB;
         break;
      case PFD_TYPE_COLORINDEX:
         *pvalue = WGL_TYPE_COLORINDEX_ARB;
         break;
      default:
         return FALSE;
      }
      break;

   case WGL_COLOR_BITS_ARB:
      *pvalue = pfi->pfd.cColorBits;
      break;

   case WGL_RED_BITS_ARB:
      *pvalue = pfi->pfd.cRedBits;
      break;

   case WGL_RED_SHIFT_ARB:
      *pvalue = pfi->pfd.cRedShift;
      break;

   case WGL_GREEN_BITS_ARB:
      *pvalue = pfi->pfd.cGreenBits;
      break;

   case WGL_GREEN_SHIFT_ARB:
      *pvalue = pfi->pfd.cGreenShift;
      break;

   case WGL_BLUE_BITS_ARB:
      *pvalue = pfi->pfd.cBlueBits;
      break;

   case WGL_BLUE_SHIFT_ARB:
      *pvalue = pfi->pfd.cBlueShift;
      break;

   case WGL_ALPHA_BITS_ARB:
      *pvalue = pfi->pfd.cAlphaBits;
      break;

   case WGL_ALPHA_SHIFT_ARB:
      *pvalue = pfi->pfd.cAlphaShift;
      break;

   case WGL_ACCUM_BITS_ARB:
      *pvalue = pfi->pfd.cAccumBits;
      break;

   case WGL_ACCUM_RED_BITS_ARB:
      *pvalue = pfi->pfd.cAccumRedBits;
      break;

   case WGL_ACCUM_GREEN_BITS_ARB:
      *pvalue = pfi->pfd.cAccumGreenBits;
      break;

   case WGL_ACCUM_BLUE_BITS_ARB:
      *pvalue = pfi->pfd.cAccumBlueBits;
      break;

   case WGL_ACCUM_ALPHA_BITS_ARB:
      *pvalue = pfi->pfd.cAccumAlphaBits;
      break;

   case WGL_DEPTH_BITS_ARB:
      *pvalue = pfi->pfd.cDepthBits;
      break;

   case WGL_STENCIL_BITS_ARB:
      *pvalue = pfi->pfd.cStencilBits;
      break;

   case WGL_AUX_BUFFERS_ARB:
      *pvalue = pfi->pfd.cAuxBuffers;
      break;

   case WGL_SAMPLE_BUFFERS_ARB:
      *pvalue = 1;
      break;

   case WGL_SAMPLES_ARB:
      *pvalue = pfi->stvis.samples;
      break;


   /* WGL_ARB_pbuffer */

   case WGL_MAX_PBUFFER_WIDTH_ARB:
   case WGL_MAX_PBUFFER_HEIGHT_ARB:
      *pvalue = stw_dev->max_2d_length;
      break;

   case WGL_MAX_PBUFFER_PIXELS_ARB:
      *pvalue = stw_dev->max_2d_length * stw_dev->max_2d_length;
      break;

   case WGL_DRAW_TO_PBUFFER_ARB:
      *pvalue = 1;
      break;


   default:
      return FALSE;
   }

   return TRUE;
}

struct attrib_match_info
{
   int attribute;
   int weight;
   BOOL exact;
};

static const struct attrib_match_info attrib_match[] = {

   /* WGL_ARB_pixel_format */
   { WGL_DRAW_TO_WINDOW_ARB,      0, TRUE },
   { WGL_DRAW_TO_BITMAP_ARB,      0, TRUE },
   { WGL_ACCELERATION_ARB,        0, TRUE },
   { WGL_NEED_PALETTE_ARB,        0, TRUE },
   { WGL_NEED_SYSTEM_PALETTE_ARB, 0, TRUE },
   { WGL_SWAP_LAYER_BUFFERS_ARB,  0, TRUE },
   { WGL_SWAP_METHOD_ARB,         0, TRUE },
   { WGL_NUMBER_OVERLAYS_ARB,     4, FALSE },
   { WGL_NUMBER_UNDERLAYS_ARB,    4, FALSE },
   /*{ WGL_SHARE_DEPTH_ARB,         0, TRUE },*/     /* no overlays -- ignore */
   /*{ WGL_SHARE_STENCIL_ARB,       0, TRUE },*/   /* no overlays -- ignore */
   /*{ WGL_SHARE_ACCUM_ARB,         0, TRUE },*/     /* no overlays -- ignore */
   { WGL_SUPPORT_GDI_ARB,         0, TRUE },
   { WGL_SUPPORT_OPENGL_ARB,      0, TRUE },
   { WGL_DOUBLE_BUFFER_ARB,       0, TRUE },
   { WGL_STEREO_ARB,              0, TRUE },
   { WGL_PIXEL_TYPE_ARB,          0, TRUE },
   { WGL_COLOR_BITS_ARB,          1, FALSE },
   { WGL_RED_BITS_ARB,            1, FALSE },
   { WGL_GREEN_BITS_ARB,          1, FALSE },
   { WGL_BLUE_BITS_ARB,           1, FALSE },
   { WGL_ALPHA_BITS_ARB,          1, FALSE },
   { WGL_ACCUM_BITS_ARB,          1, FALSE },
   { WGL_ACCUM_RED_BITS_ARB,      1, FALSE },
   { WGL_ACCUM_GREEN_BITS_ARB,    1, FALSE },
   { WGL_ACCUM_BLUE_BITS_ARB,     1, FALSE },
   { WGL_ACCUM_ALPHA_BITS_ARB,    1, FALSE },
   { WGL_DEPTH_BITS_ARB,          1, FALSE },
   { WGL_STENCIL_BITS_ARB,        1, FALSE },
   { WGL_AUX_BUFFERS_ARB,         2, FALSE },

   /* WGL_ARB_multisample */
   { WGL_SAMPLE_BUFFERS_ARB,      2, FALSE },
   { WGL_SAMPLES_ARB,             2, FALSE }
};

struct stw_pixelformat_score
{
   int points;
   uint index;
};

static BOOL
score_pixelformats(
   struct stw_pixelformat_score *scores,
   uint count,
   int attribute,
   int expected_value )
{
   uint i;
   const struct attrib_match_info *ami = NULL;
   uint index;

   /* Find out if a given attribute should be considered for score calculation.
    */
   for (i = 0; i < sizeof( attrib_match ) / sizeof( attrib_match[0] ); i++) {
      if (attrib_match[i].attribute == attribute) {
         ami = &attrib_match[i];
         break;
      }
   }
   if (ami == NULL)
      return TRUE;

   /* Iterate all pixelformats, query the requested attribute and calculate
    * score points.
    */
   for (index = 0; index < count; index++) {
      int actual_value;

      if (!stw_query_attrib( index + 1, 0, attribute, &actual_value ))
         return FALSE;

      if (ami->exact) {
         /* For an exact match criteria, if the actual and expected values differ,
          * the score is set to 0 points, effectively removing the pixelformat
          * from a list of matching pixelformats.
          */
         if (actual_value != expected_value)
            scores[index].points = 0;
      }
      else {
         /* For a minimum match criteria, if the actual value is smaller than the expected
          * value, the pixelformat is rejected (score set to 0). However, if the actual
          * value is bigger, the pixelformat is given a penalty to favour pixelformats that
          * more closely match the expected values.
          */
         if (actual_value < expected_value)
            scores[index].points = 0;
         else if (actual_value > expected_value)
            scores[index].points -= (actual_value - expected_value) * ami->weight;
      }
   }

   return TRUE;
}

WINGDIAPI BOOL APIENTRY
wglChoosePixelFormatARB(
   HDC hdc,
   const int *piAttribIList,
   const FLOAT *pfAttribFList,
   UINT nMaxFormats,
   int *piFormats,
   UINT *nNumFormats )
{
   uint count;
   struct stw_pixelformat_score *scores;
   uint i;

   *nNumFormats = 0;

   /* Allocate and initialize pixelformat score table -- better matches
    * have higher scores. Start with a high score and take out penalty
    * points for a mismatch when the match does not have to be exact.
    * Set a score to 0 if there is a mismatch for an exact match criteria.
    */
   count = stw_pixelformat_get_extended_count();
   scores = (struct stw_pixelformat_score *) MALLOC( count * sizeof( struct stw_pixelformat_score ) );
   if (scores == NULL)
      return FALSE;
   for (i = 0; i < count; i++) {
      scores[i].points = 0x7fffffff;
      scores[i].index = i;
   }

   /* Given the attribute list calculate a score for each pixelformat.
    */
   if (piAttribIList != NULL) {
      while (*piAttribIList != 0) {
         if (!score_pixelformats( scores, count, piAttribIList[0], piAttribIList[1] )) {
            FREE( scores );
            return FALSE;
         }
         piAttribIList += 2;
      }
   }
   if (pfAttribFList != NULL) {
      while (*pfAttribFList != 0) {
         if (!score_pixelformats( scores, count, (int) pfAttribFList[0], (int) pfAttribFList[1] )) {
            FREE( scores );
            return FALSE;
         }
         pfAttribFList += 2;
      }
   }

   /* Bubble-sort the resulting scores. Pixelformats with higher scores go first.
    * TODO: Find out if there are any patent issues with it.
    */
   if (count > 1) {
      uint n = count;
      boolean swapped;

      do {
         swapped = FALSE;
         for (i = 1; i < n; i++) {
            if (scores[i - 1].points < scores[i].points) {
               struct stw_pixelformat_score score = scores[i - 1];

               scores[i - 1] = scores[i];
               scores[i] = score;
               swapped = TRUE;
            }
         }
         n--;
      }
      while (swapped);
   }

   /* Return a list of pixelformats that are the best match.
    * Reject pixelformats with non-positive scores.
    */
   for (i = 0; i < count; i++) {
      if (scores[i].points > 0) {
         if (*nNumFormats < nMaxFormats)
            piFormats[*nNumFormats] = scores[i].index + 1;
         (*nNumFormats)++;
      }
   }

   FREE( scores );
   return TRUE;
}

WINGDIAPI BOOL APIENTRY
wglGetPixelFormatAttribfvARB(
   HDC hdc,
   int iPixelFormat,
   int iLayerPlane,
   UINT nAttributes,
   const int *piAttributes,
   FLOAT *pfValues )
{
   UINT i;

   (void) hdc;

   for (i = 0; i < nAttributes; i++) {
      int value;

      if (!stw_query_attrib( iPixelFormat, iLayerPlane, piAttributes[i], &value ))
         return FALSE;
      pfValues[i] = (FLOAT) value;
   }

   return TRUE;
}

WINGDIAPI BOOL APIENTRY
wglGetPixelFormatAttribivARB(
   HDC hdc,
   int iPixelFormat,
   int iLayerPlane,
   UINT nAttributes,
   const int *piAttributes,
   int *piValues )
{
   UINT i;

   (void) hdc;

   for (i = 0; i < nAttributes; i++) {
      if (!stw_query_attrib( iPixelFormat, iLayerPlane, piAttributes[i], &piValues[i] ))
         return FALSE;
   }

   return TRUE;
}
