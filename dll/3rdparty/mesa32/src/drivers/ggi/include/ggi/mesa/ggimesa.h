/* GGI-Driver for MESA
 *
 * Copyright (C) 1997  Uwe Maurer
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
 * ---------------------------------------------------------------------
 * This code was derived from the following source of information:
 *
 * svgamesa.c and ddsample.c by Brian Paul
 *
 */

#ifndef _GGIMESA_H
#define _GGIMESA_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "config.h"
#include "context.h"
#include "drawpix.h"
#include "imports.h"
#include "matrix.h"
#include "state.h"
#include "mtypes.h"
#include "macros.h"
#include "depth.h"

#undef ASSERT		/* ASSERT is redefined */

#include <ggi/internal/internal.h>
#include <ggi/ggi_ext.h>
#include <ggi/ggi.h>
#include "GL/ggimesa.h"

/*
 * GGIMesa visual configuration.
 * 
 * This structure "derives" from Mesa's GLvisual and extends it by
 * GGI's visual. Combination of these two structures is enough to fully
 * describe the mode the application is currently running in.  GGI
 * visual provides information about color configuration and buffering
 * method, GLvisual fills the rest.
 */
struct ggi_mesa_visual {
	GLvisual gl_visual;
	ggi_visual_t ggi_visual;
};

/*
 * GGIMesa context.
 *
 * GGIMesa context expands the Mesa's context (it doesn't actualy derive
 * from it, but this ability isn't needed, and it is best if GL context
 * creation is left up to Mesa). It also contains a reference to the GGI
 * visual it is attached to, which is very useful for all Mesa callbacks.
 */
struct ggi_mesa_context
{
	GLcontext *gl_ctx;
	ggi_visual_t ggi_visual;
	
	ggi_pixel color;		/* Current color or index*/
	ggi_pixel clearcolor;
	
	void *priv;
};

#define SHIFT (GGI_COLOR_PRECISION - 8)

#endif

