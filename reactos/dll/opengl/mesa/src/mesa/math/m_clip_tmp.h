/*
 * Mesa 3-D graphics library
 * Version:  6.2
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

/*
 * New (3.1) transformation code written by Keith Whitwell.
 */


/* KW: a clever asm implementation would nestle integer versions
 * of the outcode calculation underneath the division.  Gcc won't
 * do this, strangely enough, so I only do the divide in
 * the case where the cliptest passes.  This isn't essential,
 * and an asm implementation needn't replicate that behaviour.
 *
 * \param clip_vec vector of incoming clip-space coords
 * \param proj_vec vector of resultant NDC-space projected coords
 * \param clipMask resulting array of clip flags
 * \param orMask bitwise-OR of clipMask values
 * \param andMask bitwise-AND of clipMask values
 * \return proj_vec pointer
 */
static GLvector4f * _XFORMAPI TAG(cliptest_points4)( GLvector4f *clip_vec,
                                                     GLvector4f *proj_vec,
                                                     GLubyte clipMask[],
                                                     GLubyte *orMask,
                                                     GLubyte *andMask,
						     GLboolean viewport_z_clip )
{
   const GLuint stride = clip_vec->stride;
   const GLfloat *from = (GLfloat *)clip_vec->start;
   const GLuint count = clip_vec->count;
   GLuint c = 0;
   GLfloat (*vProj)[4] = (GLfloat (*)[4])proj_vec->start;
   GLubyte tmpAndMask = *andMask;
   GLubyte tmpOrMask = *orMask;
   GLuint i;
   STRIDE_LOOP {
      const GLfloat cx = from[0];
      const GLfloat cy = from[1];
      const GLfloat cz = from[2];
      const GLfloat cw = from[3];
#if defined(macintosh) || defined(__powerpc__)
      /* on powerpc cliptest is 17% faster in this way. */
      GLuint mask;
      mask = (((cw < cx) << CLIP_RIGHT_SHIFT));
      mask |= (((cw < -cx) << CLIP_LEFT_SHIFT));
      mask |= (((cw < cy) << CLIP_TOP_SHIFT));
      mask |= (((cw < -cy) << CLIP_BOTTOM_SHIFT));
      if (viewport_z_clip) {
	 mask |= (((cw < cz) << CLIP_FAR_SHIFT));
	 mask |= (((cw < -cz) << CLIP_NEAR_SHIFT));
      }
#else /* !defined(macintosh)) */
      GLubyte mask = 0;
      if (-cx + cw < 0) mask |= CLIP_RIGHT_BIT;
      if ( cx + cw < 0) mask |= CLIP_LEFT_BIT;
      if (-cy + cw < 0) mask |= CLIP_TOP_BIT;
      if ( cy + cw < 0) mask |= CLIP_BOTTOM_BIT;
      if (viewport_z_clip) {
	 if (-cz + cw < 0) mask |= CLIP_FAR_BIT;
	 if ( cz + cw < 0) mask |= CLIP_NEAR_BIT;
      }
#endif /* defined(macintosh) */

      clipMask[i] = mask;
      if (mask) {
	 c++;
	 tmpAndMask &= mask;
	 tmpOrMask |= mask;
	 vProj[i][0] = 0;
	 vProj[i][1] = 0;
	 vProj[i][2] = 0;
	 vProj[i][3] = 1;
      } else {
	 GLfloat oow = 1.0F / cw;
	 vProj[i][0] = cx * oow;
	 vProj[i][1] = cy * oow;
	 vProj[i][2] = cz * oow;
	 vProj[i][3] = oow;
      }
   }

   *orMask = tmpOrMask;
   *andMask = (GLubyte) (c < count ? 0 : tmpAndMask);

   proj_vec->flags |= VEC_SIZE_4;
   proj_vec->size = 4;
   proj_vec->count = clip_vec->count;
   return proj_vec;
}



/*
 * \param clip_vec vector of incoming clip-space coords
 * \param proj_vec vector of resultant NDC-space projected coords
 * \param clipMask resulting array of clip flags
 * \param orMask bitwise-OR of clipMask values
 * \param andMask bitwise-AND of clipMask values
 * \return clip_vec pointer
 */
static GLvector4f * _XFORMAPI TAG(cliptest_np_points4)( GLvector4f *clip_vec,
							GLvector4f *proj_vec,
							GLubyte clipMask[],
							GLubyte *orMask,
							GLubyte *andMask,
							GLboolean viewport_z_clip )
{
   const GLuint stride = clip_vec->stride;
   const GLuint count = clip_vec->count;
   const GLfloat *from = (GLfloat *)clip_vec->start;
   GLuint c = 0;
   GLubyte tmpAndMask = *andMask;
   GLubyte tmpOrMask = *orMask;
   GLuint i;
   (void) proj_vec;
   STRIDE_LOOP {
      const GLfloat cx = from[0];
      const GLfloat cy = from[1];
      const GLfloat cz = from[2];
      const GLfloat cw = from[3];
#if defined(macintosh) || defined(__powerpc__)
      /* on powerpc cliptest is 17% faster in this way. */
      GLuint mask;
      mask = (((cw < cx) << CLIP_RIGHT_SHIFT));
      mask |= (((cw < -cx) << CLIP_LEFT_SHIFT));
      mask |= (((cw < cy) << CLIP_TOP_SHIFT));
      mask |= (((cw < -cy) << CLIP_BOTTOM_SHIFT));
      if (viewport_z_clip) {
	 mask |= (((cw < cz) << CLIP_FAR_SHIFT));
	 mask |= (((cw < -cz) << CLIP_NEAR_SHIFT));
      }
#else /* !defined(macintosh)) */
      GLubyte mask = 0;
      if (-cx + cw < 0) mask |= CLIP_RIGHT_BIT;
      if ( cx + cw < 0) mask |= CLIP_LEFT_BIT;
      if (-cy + cw < 0) mask |= CLIP_TOP_BIT;
      if ( cy + cw < 0) mask |= CLIP_BOTTOM_BIT;
      if (viewport_z_clip) {
	 if (-cz + cw < 0) mask |= CLIP_FAR_BIT;
	 if ( cz + cw < 0) mask |= CLIP_NEAR_BIT;
      }
#endif /* defined(macintosh) */

      clipMask[i] = mask;
      if (mask) {
	 c++;
	 tmpAndMask &= mask;
	 tmpOrMask |= mask;
      }
   }

   *orMask = tmpOrMask;
   *andMask = (GLubyte) (c < count ? 0 : tmpAndMask);
   return clip_vec;
}


static GLvector4f * _XFORMAPI TAG(cliptest_points3)( GLvector4f *clip_vec,
                                                     GLvector4f *proj_vec,
                                                     GLubyte clipMask[],
                                                     GLubyte *orMask,
                                                     GLubyte *andMask,
						     GLboolean viewport_z_clip )
{
   const GLuint stride = clip_vec->stride;
   const GLuint count = clip_vec->count;
   const GLfloat *from = (GLfloat *)clip_vec->start;
   GLubyte tmpOrMask = *orMask;
   GLubyte tmpAndMask = *andMask;
   GLuint i;
   (void) proj_vec;
   STRIDE_LOOP {
      const GLfloat cx = from[0], cy = from[1], cz = from[2];
      GLubyte mask = 0;
      if (cx >  1.0)       mask |= CLIP_RIGHT_BIT;
      else if (cx < -1.0)  mask |= CLIP_LEFT_BIT;
      if (cy >  1.0)       mask |= CLIP_TOP_BIT;
      else if (cy < -1.0)  mask |= CLIP_BOTTOM_BIT;
      if (viewport_z_clip) {
	 if (cz >  1.0)       mask |= CLIP_FAR_BIT;
	 else if (cz < -1.0)  mask |= CLIP_NEAR_BIT;
      }
      clipMask[i] = mask;
      tmpOrMask |= mask;
      tmpAndMask &= mask;
   }

   *orMask = tmpOrMask;
   *andMask = tmpAndMask;
   return clip_vec;
}


static GLvector4f * _XFORMAPI TAG(cliptest_points2)( GLvector4f *clip_vec,
                                                     GLvector4f *proj_vec,
                                                     GLubyte clipMask[],
                                                     GLubyte *orMask,
                                                     GLubyte *andMask,
						     GLboolean viewport_z_clip )
{
   const GLuint stride = clip_vec->stride;
   const GLuint count = clip_vec->count;
   const GLfloat *from = (GLfloat *)clip_vec->start;
   GLubyte tmpOrMask = *orMask;
   GLubyte tmpAndMask = *andMask;
   GLuint i;
   (void) proj_vec;
   STRIDE_LOOP {
      const GLfloat cx = from[0], cy = from[1];
      GLubyte mask = 0;
      if (cx >  1.0)       mask |= CLIP_RIGHT_BIT;
      else if (cx < -1.0)  mask |= CLIP_LEFT_BIT;
      if (cy >  1.0)       mask |= CLIP_TOP_BIT;
      else if (cy < -1.0)  mask |= CLIP_BOTTOM_BIT;
      clipMask[i] = mask;
      tmpOrMask |= mask;
      tmpAndMask &= mask;
   }

   *orMask = tmpOrMask;
   *andMask = tmpAndMask;
   return clip_vec;
}


void TAG(init_c_cliptest)( void )
{
   _mesa_clip_tab[4] = TAG(cliptest_points4);
   _mesa_clip_tab[3] = TAG(cliptest_points3);
   _mesa_clip_tab[2] = TAG(cliptest_points2);

   _mesa_clip_np_tab[4] = TAG(cliptest_np_points4);
   _mesa_clip_np_tab[3] = TAG(cliptest_points3);
   _mesa_clip_np_tab[2] = TAG(cliptest_points2);
}
