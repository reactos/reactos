/* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * (c) Copyright IBM Corporation 2002
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
 * VA LINUX SYSTEM, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Ian Romanick <idr@us.ibm.com>
 */
/* $XFree86:$ */

#ifndef DRI_VBLANK_H
#define DRI_VBLANK_H

#include "context.h"
#include "dri_util.h"
#include "xmlconfig.h"

#define VBLANK_FLAG_INTERVAL  (1U << 0)  /* Respect the swap_interval setting
					  */
#define VBLANK_FLAG_THROTTLE  (1U << 1)  /* Wait 1 refresh since last call.
					  */
#define VBLANK_FLAG_SYNC      (1U << 2)  /* Sync to the next refresh.
					  */
#define VBLANK_FLAG_NO_IRQ    (1U << 7)  /* DRM has no IRQ to wait on.
					  */
#define VBLANK_FLAG_SECONDARY (1U << 8)  /* Wait for secondary vblank.
					  */

extern int driGetMSC32( __DRIscreenPrivate * priv, int64_t * count );
extern int driWaitForMSC32( __DRIdrawablePrivate *priv,
    int64_t target_msc, int64_t divisor, int64_t remainder, int64_t * msc );
extern GLuint driGetDefaultVBlankFlags( const driOptionCache *optionCache );
extern void driDrawableInitVBlank ( __DRIdrawablePrivate *priv, GLuint flags,
				    GLuint *vbl_seq );
extern unsigned driGetVBlankInterval( const  __DRIdrawablePrivate *priv,
				      GLuint flags );
extern void driGetCurrentVBlank( const  __DRIdrawablePrivate *priv,
				 GLuint flags, GLuint *vbl_seq );
extern int driWaitForVBlank( const __DRIdrawablePrivate *priv,
    GLuint * vbl_seq, GLuint flags, GLboolean * missed_deadline );

#undef usleep
#include <unistd.h>  /* for usleep() */
#include <sched.h>   /* for sched_yield() */

#ifdef linux
#include <sched.h>   /* for sched_yield() */
#endif

#define DO_USLEEP(nr)							\
   do {								 	\
      if (0) fprintf(stderr, "%s: usleep for %u\n", __FUNCTION__, nr );	\
      if (1) usleep( nr );						\
      sched_yield();							\
   } while( 0 )

#endif /* DRI_VBLANK_H */
