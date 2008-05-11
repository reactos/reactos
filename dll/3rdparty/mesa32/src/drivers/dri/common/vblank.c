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

#include "glheader.h"
#include "xf86drm.h"
#include "mtypes.h"
#include "macros.h"
#include "dd.h"
#include "vblank.h"
#include "xmlpool.h"


/****************************************************************************/
/**
 * Get the current MSC refresh counter.
 *
 * Stores the 64-bit count of vertical refreshes since some (arbitrary)
 * point in time in \c count.  Unless the value wraps around, which it
 * may, it will never decrease.
 *
 * \warning This function is called from \c glXGetVideoSyncSGI, which expects
 * a \c count of type \c unsigned (32-bit), and \c glXGetSyncValuesOML, which 
 * expects a \c count of type \c int64_t (signed 64-bit).  The kernel ioctl 
 * currently always returns a \c sequence of type \c unsigned.
 *
 * \param priv   Pointer to the DRI screen private struct.
 * \param count  Storage to hold MSC counter.
 * \return       Zero is returned on success.  A negative errno value
 *               is returned on failure.
 */
int driGetMSC32( __DRIscreenPrivate * priv, int64_t * count )
{
   drmVBlank vbl;
   int ret;

   /* Don't wait for anything.  Just get the current refresh count. */

   vbl.request.type = DRM_VBLANK_RELATIVE;
   vbl.request.sequence = 0;

   ret = drmWaitVBlank( priv->fd, &vbl );
   *count = (int64_t)vbl.reply.sequence;

   return ret;
}


/****************************************************************************/
/**
 * Wait for a specified refresh count.  This implements most of the
 * functionality of \c glXWaitForMscOML from the GLX_OML_sync_control spec.
 * Waits for the \c target_msc refresh.  If that has already passed, it
 * waits until \f$(MSC \bmod divisor)\f$ is equal to \c remainder.  If 
 * \c target_msc is 0, use the behavior of glXWaitVideoSyncSGI(), which
 * omits the initial check against a target MSC value.
 * 
 * This function is actually something of a hack.  The problem is that, at
 * the time of this writing, none of the existing DRM modules support an
 * ioctl that returns a 64-bit count (at least not on 32-bit platforms).
 * However, this function exists to support a GLX function that requires
 * the use of 64-bit counts.  As such, there is a little bit of ugly
 * hackery at the end of this function to make the 32-bit count act like
 * a 64-bit count.  There are still some cases where this will break, but
 * I believe it catches the most common cases.
 *
 * The real solution is to provide an ioctl that uses a 64-bit count.
 *
 * \param dpy         Pointer to the \c Display.
 * \param priv        Pointer to the DRI drawable private.
 * \param target_msc  Desired refresh count to wait for.  A value of 0
 *                    means to use the glXWaitVideoSyncSGI() behavior.
 * \param divisor     MSC divisor if \c target_msc is already reached.
 * \param remainder   Desired MSC remainder if \c target_msc is already
 *                    reached.
 * \param msc         Buffer to hold MSC when done waiting.
 *
 * \return            Zero on success or \c GLX_BAD_CONTEXT on failure.
 */

int driWaitForMSC32( __DRIdrawablePrivate *priv,
		     int64_t target_msc, int64_t divisor, int64_t remainder,
		     int64_t * msc )
{
   drmVBlank vbl;


   if ( divisor != 0 ) {
      unsigned int target = (unsigned int)target_msc;
      unsigned int next = target;
      unsigned int r;
      int dont_wait = (target_msc == 0);

      do {
         /* dont_wait means we're using the glXWaitVideoSyncSGI() behavior.
          * The first time around, just get the current count and proceed 
          * to the test for (MSC % divisor) == remainder.
          */
         vbl.request.type = dont_wait ? DRM_VBLANK_RELATIVE :
                                        DRM_VBLANK_ABSOLUTE;
         vbl.request.sequence = next;

	 if ( drmWaitVBlank( priv->driScreenPriv->fd, &vbl ) != 0 ) {
	    /* FIXME: This doesn't seem like the right thing to return here.
	     */
	    return GLX_BAD_CONTEXT;
	 }

         dont_wait = 0;
         if (target_msc != 0 && vbl.reply.sequence == target)
            break;

         /* Assuming the wait-done test fails, the next refresh to wait for
          * will be one that satisfies (MSC % divisor) == remainder.  The
          * value (MSC - (MSC % divisor) + remainder) is the refresh value 
          * closest to the current value that would satisfy the equation.  
          * If this refresh has already happened, we add divisor to obtain 
          * the next refresh after the current one that will satisfy it.
          */
         r = (vbl.reply.sequence % (unsigned int)divisor);
         next = (vbl.reply.sequence - r + (unsigned int)remainder);
         if (next <= vbl.reply.sequence) next += (unsigned int)divisor;

      } while ( r != (unsigned int)remainder );
   }
   else {
      /* If the \c divisor is zero, just wait until the MSC is greater
       * than or equal to \c target_msc.
       */

      vbl.request.type = DRM_VBLANK_ABSOLUTE;
      vbl.request.sequence = target_msc;

      if ( drmWaitVBlank( priv->driScreenPriv->fd, &vbl ) != 0 ) {
	 /* FIXME: This doesn't seem like the right thing to return here.
	  */
	 return GLX_BAD_CONTEXT;
      }
   }

   *msc  = (target_msc & 0xffffffff00000000LL);
   *msc |= vbl.reply.sequence;
   if ( *msc < target_msc ) {
      *msc += 0x0000000100000000LL;
   }

   return 0;
}


/****************************************************************************/
/**
 * Gets a set of default vertical-blank-wait flags based on the internal GLX
 * API version and several configuration options.
 */

GLuint driGetDefaultVBlankFlags( const driOptionCache *optionCache )
{
   GLuint  flags = VBLANK_FLAG_INTERVAL;
   int vblank_mode;


   if ( driCheckOption( optionCache, "vblank_mode", DRI_ENUM ) )
      vblank_mode = driQueryOptioni( optionCache, "vblank_mode" );
   else
      vblank_mode = DRI_CONF_VBLANK_DEF_INTERVAL_1;

   switch (vblank_mode) {
   case DRI_CONF_VBLANK_NEVER:
      flags = 0;
      break;
   case DRI_CONF_VBLANK_DEF_INTERVAL_0:
      break;
   case DRI_CONF_VBLANK_DEF_INTERVAL_1:
      flags |= VBLANK_FLAG_THROTTLE;
      break;
   case DRI_CONF_VBLANK_ALWAYS_SYNC:
      flags |= VBLANK_FLAG_SYNC;
      break;
   }

   return flags;
}


/****************************************************************************/
/**
 * Wrapper to call \c drmWaitVBlank.  The main purpose of this function is to
 * wrap the error message logging.  The error message should only be logged
 * the first time the \c drmWaitVBlank fails.  If \c drmWaitVBlank is
 * successful, \c vbl_seq will be set the sequence value in the reply.
 *
 * \param vbl      Pointer to drmVBlank packet desribing how to wait.
 * \param vbl_seq  Location to store the current refresh counter.
 * \param fd       File descriptor use to call into the DRM.
 * \return         Zero on success or -1 on failure.
 */

static int do_wait( drmVBlank * vbl, GLuint * vbl_seq, int fd )
{
   int   ret;


   ret = drmWaitVBlank( fd, vbl );
   if ( ret != 0 ) {
      static GLboolean first_time = GL_TRUE;

      if ( first_time ) {
	 fprintf(stderr, 
		 "%s: drmWaitVBlank returned %d, IRQs don't seem to be"
		 " working correctly.\nTry running with LIBGL_THROTTLE_REFRESH"
		 " and LIBL_SYNC_REFRESH unset.\n", __FUNCTION__, ret);
	 first_time = GL_FALSE;
      }

      return -1;
   }

   *vbl_seq = vbl->reply.sequence;
   return 0;
}


/****************************************************************************/
/**
 * Sets the default swap interval when the drawable is first bound to a
 * direct rendering context.
 */

void driDrawableInitVBlank( __DRIdrawablePrivate *priv, GLuint flags,
			    GLuint *vbl_seq )
{
   if ( priv->pdraw->swap_interval == (unsigned)-1 ) {
      /* Get current vertical blank sequence */
      drmVBlank vbl = { .request={ .type = DRM_VBLANK_RELATIVE, .sequence = 0 } };
      do_wait( &vbl, vbl_seq, priv->driScreenPriv->fd );

      priv->pdraw->swap_interval = (flags & (VBLANK_FLAG_THROTTLE |
					     VBLANK_FLAG_SYNC)) != 0 ? 1 : 0;
   }
}


/****************************************************************************/
/**
 * Returns the current swap interval of the given drawable.
 */

unsigned
driGetVBlankInterval( const  __DRIdrawablePrivate *priv, GLuint flags )
{
   if ( (flags & VBLANK_FLAG_INTERVAL) != 0 ) {
      /* this must have been initialized when the drawable was first bound
       * to a direct rendering context. */
      assert ( priv->pdraw->swap_interval != (unsigned)-1 );

      return priv->pdraw->swap_interval;
   }
   else if ( (flags & (VBLANK_FLAG_THROTTLE | VBLANK_FLAG_SYNC)) != 0 ) {
      return 1;
   }
   else {
      return 0;
   }
}


/****************************************************************************/
/**
 * Returns the current vertical blank sequence number of the given drawable.
 */

void
driGetCurrentVBlank( const  __DRIdrawablePrivate *priv, GLuint flags,
		     GLuint *vbl_seq )
{
   drmVBlank vbl;

   vbl.request.type = DRM_VBLANK_RELATIVE;
   if ( flags & VBLANK_FLAG_SECONDARY ) {
      vbl.request.type |= DRM_VBLANK_SECONDARY;
   }
   vbl.request.sequence = 0;

   (void) do_wait( &vbl, vbl_seq, priv->driScreenPriv->fd );
}


/****************************************************************************/
/**
 * Waits for the vertical blank for use with glXSwapBuffers.
 * 
 * \param vbl_seq  Vertical blank sequence number (MSC) after the last buffer
 *                 swap.  Updated after this wait.
 * \param flags    \c VBLANK_FLAG bits that control how long to wait.
 * \param missed_deadline  Set to \c GL_TRUE if the MSC after waiting is later
 *                 than the "target" based on \c flags.  The idea is that if
 *                 \c missed_deadline is set, then the application is not 
 *                 achieving its desired framerate.
 * \return         Zero on success, -1 on error.
 */

int
driWaitForVBlank( const  __DRIdrawablePrivate *priv, GLuint * vbl_seq,
		  GLuint flags, GLboolean * missed_deadline )
{
   drmVBlank vbl;
   unsigned   original_seq;
   unsigned   deadline;
   unsigned   interval;
   unsigned   diff;

   *missed_deadline = GL_FALSE;
   if ( (flags & (VBLANK_FLAG_INTERVAL |
		  VBLANK_FLAG_THROTTLE |
		  VBLANK_FLAG_SYNC)) == 0 ||
	(flags & VBLANK_FLAG_NO_IRQ) != 0 ) {
      return 0;
   }


   /* VBLANK_FLAG_SYNC means to wait for at least one vertical blank.  If
    * that flag is not set, do a fake wait for zero vertical blanking
    * periods so that we can get the current MSC.
    *
    * VBLANK_FLAG_INTERVAL and VBLANK_FLAG_THROTTLE mean to wait for at
    * least one vertical blank since the last wait.  Since do_wait modifies
    * vbl_seq, we have to save the original value of vbl_seq for the
    * VBLANK_FLAG_INTERVAL / VBLANK_FLAG_THROTTLE calculation later.
    */

   original_seq = *vbl_seq;
   interval = driGetVBlankInterval(priv, flags);
   deadline = original_seq + interval;

   vbl.request.type = DRM_VBLANK_RELATIVE;
   if ( flags & VBLANK_FLAG_SECONDARY ) {
      vbl.request.type |= DRM_VBLANK_SECONDARY;
   }
   vbl.request.sequence = ((flags & VBLANK_FLAG_SYNC) != 0) ? 1 : 0;

   if ( do_wait( & vbl, vbl_seq, priv->driScreenPriv->fd ) != 0 ) {
      return -1;
   }

   diff = *vbl_seq - deadline;

   /* No need to wait again if we've already reached the target */
   if (diff <= (1 << 23)) {
      *missed_deadline = (flags & VBLANK_FLAG_SYNC) ? (diff > 0) : GL_TRUE;
      return 0;
   }

   /* Wait until the target vertical blank. */
   vbl.request.type = DRM_VBLANK_ABSOLUTE;
   if ( flags & VBLANK_FLAG_SECONDARY ) {
      vbl.request.type |= DRM_VBLANK_SECONDARY;
   }
   vbl.request.sequence = deadline;

   if ( do_wait( & vbl, vbl_seq, priv->driScreenPriv->fd ) != 0 ) {
      return -1;
   }

   diff = *vbl_seq - deadline;
   *missed_deadline = diff > 0 && diff <= (1 << 23);

   return 0;
}
