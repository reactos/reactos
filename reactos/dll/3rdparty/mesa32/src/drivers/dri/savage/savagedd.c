/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#include "mtypes.h"
#include "framebuffer.h"

#include <stdio.h>

#include "mm.h"
#include "swrast/swrast.h"

#include "savagedd.h"
#include "savagestate.h"
#include "savagespan.h"
#include "savagetex.h"
#include "savagetris.h"
#include "savagecontext.h"
#include "extensions.h"

#include "utils.h"


#define DRIVER_DATE "20061110"

/***************************************
 * Mesa's Driver Functions
 ***************************************/


static const GLubyte *savageDDGetString( GLcontext *ctx, GLenum name )
{
   static char *cardNames[S3_LAST] = {
       "Unknown",
       "Savage3D",
       "Savage/MX/IX",
       "Savage4",
       "ProSavage",
       "Twister",
       "ProSavageDDR",
       "SuperSavage",
       "Savage2000"
   };
   static char buffer[128];
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   savageScreenPrivate *screen = imesa->savageScreen;
   enum S3CHIPTAGS chipset = screen->chipset;
   unsigned offset;

   if (chipset < S3_SAVAGE3D || chipset >= S3_LAST)
      chipset = S3_UNKNOWN; /* should not happen */

   switch (name) {
   case GL_VENDOR:
      return (GLubyte *)"S3 Graphics Inc.";
   case GL_RENDERER:
      offset = driGetRendererString( buffer, cardNames[chipset], DRIVER_DATE,
				     screen->agpMode );
      return (GLubyte *)buffer;
   default:
      return 0;
   }
}
#if 0
static GLint savageGetParameteri(const GLcontext *ctx, GLint param)
{
   switch (param) {
   case DD_HAVE_HARDWARE_FOG:
      return 1;
   default:
      return 0;
   }
}
#endif


void savageDDInitDriverFuncs( GLcontext *ctx )
{
   ctx->Driver.GetString = savageDDGetString;
}
