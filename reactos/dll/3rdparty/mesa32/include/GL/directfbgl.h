/*
   (c) Copyright 2001  convergence integrated media GmbH.
   All rights reserved.

   Written by Denis Oliver Kropp <dok@convergence.de> and
              Andreas Hundt <andi@convergence.de>.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __DIRECTFBGL_H__
#define __DIRECTFBGL_H__

#include <directfb.h>

#ifdef __cplusplus
extern "C"
{
#endif


typedef struct {
     int        buffer_size;
     int        depth_size;
     int        stencil_size;
     int        aux_buffers;

     int        red_size;
     int        green_size;
     int        blue_size;
     int        alpha_size;

     int        accum_red_size;
     int        accum_green_size;
     int        accum_blue_size;
     int        accum_alpha_size;

     DFBBoolean double_buffer;
     DFBBoolean stereo;
} DFBGLAttributes;


DEFINE_INTERFACE(   IDirectFBGL,

   /** Context handling **/

     /*
      * Acquire the hardware lock.
      */
     DFBResult (*Lock) (
          IDirectFBGL              *thiz
     );

     /*
      * Release the lock.
      */
     DFBResult (*Unlock) (
          IDirectFBGL              *thiz
     );

     /*
      * Query the OpenGL attributes.
      */
     DFBResult (*GetAttributes) (
          IDirectFBGL              *thiz,
          DFBGLAttributes          *attributes
     );
)


#ifdef __cplusplus
}
#endif

#endif

