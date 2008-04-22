/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgadd.c,v 1.14 2002/10/30 12:51:35 alanh Exp $ */


#include "mtypes.h"
#include "framebuffer.h"

#include "mm.h"
#include "mgacontext.h"
#include "mgadd.h"
#include "mgastate.h"
#include "mgaspan.h"
#include "mgatex.h"
#include "mgatris.h"
#include "mgavb.h"
#include "mga_xmesa.h"
#include "utils.h"

#define DRIVER_DATE	"20061030"


/***************************************
 * Mesa's Driver Functions
 ***************************************/


static const GLubyte *mgaGetString( GLcontext *ctx, GLenum name )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   static char buffer[128];
   unsigned   offset;

   switch ( name ) {
   case GL_VENDOR:
      return (GLubyte *) "VA Linux Systems Inc.";

   case GL_RENDERER:
      offset = driGetRendererString( buffer, 
				     MGA_IS_G400(mmesa) ? "G400" :
				     MGA_IS_G200(mmesa) ? "G200" : "MGA",
				     DRIVER_DATE,
				     mmesa->mgaScreen->agpMode );

      return (GLubyte *)buffer;

   default:
      return NULL;
   }
}


void mgaInitDriverFuncs( struct dd_function_table *functions )
{
   functions->GetString = mgaGetString;
}
