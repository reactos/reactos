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

#include "pipe/p_format.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"

#include "util/u_format.h"
#include "util/u_debug.h"
#include "util/u_memory.h"

#include "stw_icd.h"
#include "stw_device.h"
#include "stw_pixelformat.h"
#include "stw_tls.h"


struct stw_pf_color_info
{
   enum pipe_format format;
   struct {
      unsigned char red;
      unsigned char green;
      unsigned char blue;
      unsigned char alpha;
   } bits;
   struct {
      unsigned char red;
      unsigned char green;
      unsigned char blue;
      unsigned char alpha;
   } shift;
};

struct stw_pf_depth_info
{
   enum pipe_format format;
   struct {
      unsigned char depth;
      unsigned char stencil;
   } bits;
};


/* NOTE: order matters, since in otherwise equal circumstances the first
 * format listed will get chosen */

static const struct stw_pf_color_info
stw_pf_color[] = {
   /* no-alpha */
   { PIPE_FORMAT_B8G8R8X8_UNORM,    { 8,  8,  8,  0}, {16,  8,  0,  0} },
   { PIPE_FORMAT_X8R8G8B8_UNORM,    { 8,  8,  8,  0}, { 8, 16, 24,  0} },
   { PIPE_FORMAT_B5G6R5_UNORM,      { 5,  6,  5,  0}, {11,  5,  0,  0} },
   /* alpha */
   { PIPE_FORMAT_B8G8R8A8_UNORM,    { 8,  8,  8,  8}, {16,  8,  0, 24} },
   { PIPE_FORMAT_A8R8G8B8_UNORM,    { 8,  8,  8,  8}, { 8, 16, 24,  0} },
#if 0
   { PIPE_FORMAT_R10G10B10A2_UNORM, {10, 10, 10,  2}, { 0, 10, 20, 30} },
#endif
   { PIPE_FORMAT_B5G5R5A1_UNORM,    { 5,  5,  5,  1}, {10,  5,  0, 15} },
   { PIPE_FORMAT_B4G4R4A4_UNORM,    { 4,  4,  4,  4}, {16,  4,  0, 12} }
};


static const struct stw_pf_depth_info 
stw_pf_depth_stencil[] = {
   /* pure depth */
   { PIPE_FORMAT_Z32_UNORM,   {32, 0} },
   { PIPE_FORMAT_X8Z24_UNORM, {24, 0} },
   { PIPE_FORMAT_Z24X8_UNORM, {24, 0} },
   { PIPE_FORMAT_Z16_UNORM,   {16, 0} },
   /* combined depth-stencil */
   { PIPE_FORMAT_Z24_UNORM_S8_UINT, {24, 8} },
   { PIPE_FORMAT_S8_UINT_Z24_UNORM, {24, 8} }
};


static const boolean 
stw_pf_doublebuffer[] = {
   FALSE,
   TRUE,
};


const unsigned 
stw_pf_multisample[] = {
   0,
   4
};


static void
stw_pixelformat_add(
   struct stw_device *stw_dev,
   const struct stw_pf_color_info *color,
   const struct stw_pf_depth_info *depth,
   unsigned accum,
   boolean doublebuffer,
   unsigned samples )
{
   boolean extended = FALSE;
   struct stw_pixelformat_info *pfi;
   
   assert(stw_dev->pixelformat_extended_count < STW_MAX_PIXELFORMATS);
   if(stw_dev->pixelformat_extended_count >= STW_MAX_PIXELFORMATS)
      return;

   assert(util_format_get_component_bits(color->format, UTIL_FORMAT_COLORSPACE_RGB, 0) == color->bits.red);
   assert(util_format_get_component_bits(color->format, UTIL_FORMAT_COLORSPACE_RGB, 1) == color->bits.green);
   assert(util_format_get_component_bits(color->format, UTIL_FORMAT_COLORSPACE_RGB, 2) == color->bits.blue);
   assert(util_format_get_component_bits(color->format, UTIL_FORMAT_COLORSPACE_RGB, 3) == color->bits.alpha);
   assert(util_format_get_component_bits(depth->format, UTIL_FORMAT_COLORSPACE_ZS, 0) == depth->bits.depth);
   assert(util_format_get_component_bits(depth->format, UTIL_FORMAT_COLORSPACE_ZS, 1) == depth->bits.stencil);
   
   pfi = &stw_dev->pixelformats[stw_dev->pixelformat_extended_count];
   
   memset(pfi, 0, sizeof *pfi);
   
   pfi->pfd.nSize = sizeof pfi->pfd;
   pfi->pfd.nVersion = 1;

   pfi->pfd.dwFlags = PFD_SUPPORT_OPENGL;
   
   /* TODO: also support non-native pixel formats */
   pfi->pfd.dwFlags |= PFD_DRAW_TO_WINDOW;

   /* See http://www.opengl.org/pipeline/article/vol003_7/ */
   pfi->pfd.dwFlags |= PFD_SUPPORT_COMPOSITION;

   if (doublebuffer)
      pfi->pfd.dwFlags |= PFD_DOUBLEBUFFER | PFD_SWAP_COPY;
   
   pfi->pfd.iPixelType = PFD_TYPE_RGBA;

   pfi->pfd.cColorBits = color->bits.red + color->bits.green + color->bits.blue + color->bits.alpha;
   pfi->pfd.cRedBits = color->bits.red;
   pfi->pfd.cRedShift = color->shift.red;
   pfi->pfd.cGreenBits = color->bits.green;
   pfi->pfd.cGreenShift = color->shift.green;
   pfi->pfd.cBlueBits = color->bits.blue;
   pfi->pfd.cBlueShift = color->shift.blue;
   pfi->pfd.cAlphaBits = color->bits.alpha;
   pfi->pfd.cAlphaShift = color->shift.alpha;
   pfi->pfd.cAccumBits = 4*accum;
   pfi->pfd.cAccumRedBits = accum;
   pfi->pfd.cAccumGreenBits = accum;
   pfi->pfd.cAccumBlueBits = accum;
   pfi->pfd.cAccumAlphaBits = accum;
   pfi->pfd.cDepthBits = depth->bits.depth;
   pfi->pfd.cStencilBits = depth->bits.stencil;
   pfi->pfd.cAuxBuffers = 0;
   pfi->pfd.iLayerType = 0;
   pfi->pfd.bReserved = 0;
   pfi->pfd.dwLayerMask = 0;
   pfi->pfd.dwVisibleMask = 0;
   pfi->pfd.dwDamageMask = 0;

   /*
    * since state trackers can allocate depth/stencil/accum buffers, we provide
    * only color buffers here
    */
   pfi->stvis.buffer_mask = ST_ATTACHMENT_FRONT_LEFT_MASK;
   if (doublebuffer)
      pfi->stvis.buffer_mask |= ST_ATTACHMENT_BACK_LEFT_MASK;

   pfi->stvis.color_format = color->format;
   pfi->stvis.depth_stencil_format = depth->format;

   pfi->stvis.accum_format = (accum) ?
      PIPE_FORMAT_R16G16B16A16_SNORM : PIPE_FORMAT_NONE;

   pfi->stvis.samples = samples;
   pfi->stvis.render_buffer = ST_ATTACHMENT_INVALID;
   
   ++stw_dev->pixelformat_extended_count;
   
   if(!extended) {
      ++stw_dev->pixelformat_count;
      assert(stw_dev->pixelformat_count == stw_dev->pixelformat_extended_count);
   }
}

void
stw_pixelformat_init( void )
{
   struct pipe_screen *screen = stw_dev->screen;
   unsigned i, j, k, l;
   
   assert( !stw_dev->pixelformat_count );
   assert( !stw_dev->pixelformat_extended_count );

   for(i = 0; i < Elements(stw_pf_multisample); ++i) {
      unsigned samples = stw_pf_multisample[i];
      
      /* FIXME: re-enabled MSAA when we can query it */
      if(samples)
         continue;

      for(j = 0; j < Elements(stw_pf_color); ++j) {
         const struct stw_pf_color_info *color = &stw_pf_color[j];
         
         if(!screen->is_format_supported(screen, color->format, PIPE_TEXTURE_2D,
                                         0, PIPE_BIND_RENDER_TARGET |
                                         PIPE_BIND_DISPLAY_TARGET))
            continue;
         
         for(k = 0; k < Elements(stw_pf_doublebuffer); ++k) {
            unsigned doublebuffer = stw_pf_doublebuffer[k];
            
            for(l = 0; l < Elements(stw_pf_depth_stencil); ++l) {
               const struct stw_pf_depth_info *depth = &stw_pf_depth_stencil[l];
               
               if(!screen->is_format_supported(screen, depth->format, PIPE_TEXTURE_2D, 
                                               0, PIPE_BIND_DEPTH_STENCIL))
                  continue;

               stw_pixelformat_add( stw_dev, color, depth,  0, doublebuffer, samples );
               stw_pixelformat_add( stw_dev, color, depth, 16, doublebuffer, samples );
            }
         }
      }
   }
   
   assert( stw_dev->pixelformat_count <= stw_dev->pixelformat_extended_count );
   assert( stw_dev->pixelformat_extended_count <= STW_MAX_PIXELFORMATS );
}

uint
stw_pixelformat_get_count( void )
{
   return stw_dev->pixelformat_count;
}

uint
stw_pixelformat_get_extended_count( void )
{
   return stw_dev->pixelformat_extended_count;
}

const struct stw_pixelformat_info *
stw_pixelformat_get_info( uint index )
{
   assert( index < stw_dev->pixelformat_extended_count );

   return &stw_dev->pixelformats[index];
}


LONG APIENTRY
DrvDescribePixelFormat(
   HDC hdc,
   INT iPixelFormat,
   ULONG cjpfd,
   PIXELFORMATDESCRIPTOR *ppfd )
{
   uint count;
   uint index;
   const struct stw_pixelformat_info *pfi;

   (void) hdc;

   if (!stw_dev)
      return 0;

   count = stw_pixelformat_get_extended_count();
   index = (uint) iPixelFormat - 1;

   if (ppfd == NULL)
      return count;
   if (index >= count || cjpfd != sizeof( PIXELFORMATDESCRIPTOR ))
      return 0;

   pfi = stw_pixelformat_get_info( index );
   
   memcpy(ppfd, &pfi->pfd, sizeof( PIXELFORMATDESCRIPTOR ));

   return count;
}

BOOL APIENTRY
DrvDescribeLayerPlane(
   HDC hdc,
   INT iPixelFormat,
   INT iLayerPlane,
   UINT nBytes,
   LPLAYERPLANEDESCRIPTOR plpd )
{
   assert(0);
   return FALSE;
}

int APIENTRY
DrvGetLayerPaletteEntries(
   HDC hdc,
   INT iLayerPlane,
   INT iStart,
   INT cEntries,
   COLORREF *pcr )
{
   assert(0);
   return 0;
}

int APIENTRY
DrvSetLayerPaletteEntries(
   HDC hdc,
   INT iLayerPlane,
   INT iStart,
   INT cEntries,
   CONST COLORREF *pcr )
{
   assert(0);
   return 0;
}

BOOL APIENTRY
DrvRealizeLayerPalette(
   HDC hdc,
   INT iLayerPlane,
   BOOL bRealize )
{
   assert(0);
   return FALSE;
}

/* Only used by the wgl code, but have it here to avoid exporting the
 * pixelformat.h functionality.
 */
int stw_pixelformat_choose( HDC hdc,
                            CONST PIXELFORMATDESCRIPTOR *ppfd )
{
   uint count;
   uint index;
   uint bestindex;
   uint bestdelta;

   (void) hdc;

   count = stw_pixelformat_get_count();
   bestindex = count;
   bestdelta = ~0U;

   for (index = 0; index < count; index++) {
      uint delta = 0;
      const struct stw_pixelformat_info *pfi = stw_pixelformat_get_info( index );

      if (!(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE) &&
          !!(ppfd->dwFlags & PFD_DOUBLEBUFFER) !=
          !!(pfi->pfd.dwFlags & PFD_DOUBLEBUFFER))
         continue;

      /* FIXME: Take in account individual channel bits */
      if (ppfd->cColorBits != pfi->pfd.cColorBits)
         delta += 8;

      if (ppfd->cDepthBits != pfi->pfd.cDepthBits)
         delta += 4;

      if (ppfd->cStencilBits != pfi->pfd.cStencilBits)
         delta += 2;

      if (ppfd->cAlphaBits != pfi->pfd.cAlphaBits)
         delta++;

      if (delta < bestdelta) {
         bestindex = index;
         bestdelta = delta;
         if (bestdelta == 0)
            break;
      }
   }

   if (bestindex == count)
      return 0;

   return bestindex + 1;
}
