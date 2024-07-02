/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

static void
FN_NAME(DST_TYPE *dst,
	GLenum dstFormat,
	SRC_TYPE rgba[][4],
	int n)
{
   int i;

   switch (dstFormat) {
   case GL_RED_INTEGER_EXT:
      for (i=0;i<n;i++) {
	 dst[i] = SRC_CONVERT(rgba[i][RCOMP]);
      }
      break;

   case GL_GREEN_INTEGER_EXT:
      for (i=0;i<n;i++) {
	 dst[i] = SRC_CONVERT(rgba[i][GCOMP]);
      }
      break;

   case GL_BLUE_INTEGER_EXT:
      for (i=0;i<n;i++) {
	 dst[i] = SRC_CONVERT(rgba[i][BCOMP]);
      };
      break;

   case GL_ALPHA_INTEGER_EXT:
      for (i=0;i<n;i++) {
	 dst[i] = SRC_CONVERT(rgba[i][ACOMP]);
      }
      break;

   case GL_RG_INTEGER:
      for (i=0;i<n;i++) {
	 dst[i*2+0] = SRC_CONVERT(rgba[i][RCOMP]);
	 dst[i*2+1] = SRC_CONVERT(rgba[i][GCOMP]);
      }
      break;

   case GL_RGB_INTEGER_EXT:
      for (i=0;i<n;i++) {
	 dst[i*3+0] = SRC_CONVERT(rgba[i][RCOMP]);
	 dst[i*3+1] = SRC_CONVERT(rgba[i][GCOMP]);
	 dst[i*3+2] = SRC_CONVERT(rgba[i][BCOMP]);
      }
      break;

   case GL_RGBA_INTEGER_EXT:
      for (i=0;i<n;i++) {
	 dst[i*4+0] = SRC_CONVERT(rgba[i][RCOMP]);
	 dst[i*4+1] = SRC_CONVERT(rgba[i][GCOMP]);
	 dst[i*4+2] = SRC_CONVERT(rgba[i][BCOMP]);
	 dst[i*4+3] = SRC_CONVERT(rgba[i][ACOMP]);
      }
      break;

   case GL_BGR_INTEGER_EXT:
      for (i=0;i<n;i++) {
	 dst[i*3+0] = SRC_CONVERT(rgba[i][BCOMP]);
	 dst[i*3+1] = SRC_CONVERT(rgba[i][GCOMP]);
	 dst[i*3+2] = SRC_CONVERT(rgba[i][RCOMP]);
      }
      break;

   case GL_BGRA_INTEGER_EXT:
      for (i=0;i<n;i++) {
	 dst[i*4+0] = SRC_CONVERT(rgba[i][BCOMP]);
	 dst[i*4+1] = SRC_CONVERT(rgba[i][GCOMP]);
	 dst[i*4+2] = SRC_CONVERT(rgba[i][RCOMP]);
	 dst[i*4+3] = SRC_CONVERT(rgba[i][ACOMP]);
      }
      break;

   case GL_LUMINANCE_INTEGER_EXT:
      for (i=0;i<n;i++) {
	 dst[i] = SRC_CONVERT(rgba[i][RCOMP] +
			      rgba[i][GCOMP] +
			      rgba[i][BCOMP]);
      }
      break;

   case GL_LUMINANCE_ALPHA_INTEGER_EXT:
      for (i=0;i<n;i++) {
	 dst[i*2+0] = SRC_CONVERT(rgba[i][RCOMP] +
				  rgba[i][GCOMP] +
				  rgba[i][BCOMP]);
	 dst[i*2+1] = SRC_CONVERT(rgba[i][ACOMP]);
      }
      break;
   }
}
