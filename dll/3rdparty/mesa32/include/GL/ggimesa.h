/*
 * Mesa 3-D graphics library GGI bindings (GGIGL [giggle])
 * Version:  4.0
 * Copyright (C) 1995-2000  Brian Paul
 * Copyright (C) 1998  Uwe Maurer
 * Copyrigth (C) 2001 Filip Spacek
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifndef GGIMESA_H
#define GGIMESA_H

#define GGIMESA_MAJOR_VERSION 4
#define GGIMESA_MINOR_VERSION 0

#ifdef __cplusplus
extern "C" {
#endif

#include <ggi/ggi.h>
#include "GL/gl.h"
  
typedef struct ggi_mesa_context *ggi_mesa_context_t;

/*
 * Initialize Mesa GGI extension
 */
int ggiMesaInit(void);
/*
 * Clean up Mesa GGI exension
 */
int ggiMesaExit(void);

/*
 * Attach Mesa GGI extension to the visual 'vis'
 */
int ggiMesaAttach(ggi_visual_t vis);
/*
 * Detach Mesa GGI extension from the visual 'vis'
 */
int ggiMesaDetach(ggi_visual_t vis);

int ggiMesaExtendVisual(ggi_visual_t vis, GLboolean alpha_flag,
			GLboolean stereo_flag, GLint depth_size,
			GLint stencil_size, GLint accum_red_size,
			GLint accum_green_size, GLint accum_blue_size,
			GLint accum_alpha_size, GLint num_samples);

/*
 * Create a new context capable of displaying on the visual vis.
 */
ggi_mesa_context_t ggiMesaCreateContext(ggi_visual_t vis);
/*
 * Destroy the context 'ctx'
 */
void ggiMesaDestroyContext(ggi_mesa_context_t ctx);

/*
 * Make context 'ctx' the current context and bind it to visual 'vis'.
 * Note that the context must have been created with respect to that visual.
 */
void ggiMesaMakeCurrent(ggi_mesa_context_t ctx, ggi_visual_t vis);

void ggiMesaSwapBuffers(void);


#ifdef __cplusplus
}
#endif

#endif
