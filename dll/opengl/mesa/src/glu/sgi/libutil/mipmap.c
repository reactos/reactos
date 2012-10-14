/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

#include "gluos.h"
#include <assert.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>		/* UINT_MAX */
#include <math.h>

typedef union {
    unsigned char ub[4];
    unsigned short us[2];
    unsigned int ui;
    char b[4];
    short s[2];
    int i;
    float f;
} Type_Widget;

/* Pixel storage modes */
typedef struct {
   GLint pack_alignment;
   GLint pack_row_length;
   GLint pack_skip_rows;
   GLint pack_skip_pixels;
   GLint pack_lsb_first;
   GLint pack_swap_bytes;
   GLint pack_skip_images;
   GLint pack_image_height;

   GLint unpack_alignment;
   GLint unpack_row_length;
   GLint unpack_skip_rows;
   GLint unpack_skip_pixels;
   GLint unpack_lsb_first;
   GLint unpack_swap_bytes;
   GLint unpack_skip_images;
   GLint unpack_image_height;
} PixelStorageModes;

static int gluBuild1DMipmapLevelsCore(GLenum, GLint,
				      GLsizei,
				      GLsizei,
				      GLenum, GLenum, GLint, GLint, GLint,
				      const void *);
static int gluBuild2DMipmapLevelsCore(GLenum, GLint,
				      GLsizei, GLsizei,
				      GLsizei, GLsizei,
				      GLenum, GLenum, GLint, GLint, GLint,
				      const void *);
static int gluBuild3DMipmapLevelsCore(GLenum, GLint,
				      GLsizei, GLsizei, GLsizei,
				      GLsizei, GLsizei, GLsizei,
				      GLenum, GLenum, GLint, GLint, GLint,
				      const void *);

/*
 * internal function declarations
 */
static GLfloat bytes_per_element(GLenum type);
static GLint elements_per_group(GLenum format, GLenum type);
static GLint is_index(GLenum format);
static GLint image_size(GLint width, GLint height, GLenum format, GLenum type);
static void fill_image(const PixelStorageModes *,
		       GLint width, GLint height, GLenum format,
		       GLenum type, GLboolean index_format,
		       const void *userdata, GLushort *newimage);
static void empty_image(const PixelStorageModes *,
			GLint width, GLint height, GLenum format,
			GLenum type, GLboolean index_format,
			const GLushort *oldimage, void *userdata);
static void scale_internal(GLint components, GLint widthin, GLint heightin,
			   const GLushort *datain,
			   GLint widthout, GLint heightout,
			   GLushort *dataout);

static void scale_internal_ubyte(GLint components, GLint widthin,
			   GLint heightin, const GLubyte *datain,
			   GLint widthout, GLint heightout,
			   GLubyte *dataout, GLint element_size,
			   GLint ysize, GLint group_size);
static void scale_internal_byte(GLint components, GLint widthin,
			   GLint heightin, const GLbyte *datain,
			   GLint widthout, GLint heightout,
			   GLbyte *dataout, GLint element_size,
			   GLint ysize, GLint group_size);
static void scale_internal_ushort(GLint components, GLint widthin,
			   GLint heightin, const GLushort *datain,
			   GLint widthout, GLint heightout,
			   GLushort *dataout, GLint element_size,
			   GLint ysize, GLint group_size,
			   GLint myswap_bytes);
static void scale_internal_short(GLint components, GLint widthin,
			   GLint heightin, const GLshort *datain,
			   GLint widthout, GLint heightout,
			   GLshort *dataout, GLint element_size,
			   GLint ysize, GLint group_size,
			   GLint myswap_bytes);
static void scale_internal_uint(GLint components, GLint widthin,
			   GLint heightin, const GLuint *datain,
			   GLint widthout, GLint heightout,
			   GLuint *dataout, GLint element_size,
			   GLint ysize, GLint group_size,
			   GLint myswap_bytes);
static void scale_internal_int(GLint components, GLint widthin,
			   GLint heightin, const GLint *datain,
			   GLint widthout, GLint heightout,
			   GLint *dataout, GLint element_size,
			   GLint ysize, GLint group_size,
			   GLint myswap_bytes);
static void scale_internal_float(GLint components, GLint widthin,
			   GLint heightin, const GLfloat *datain,
			   GLint widthout, GLint heightout,
			   GLfloat *dataout, GLint element_size,
			   GLint ysize, GLint group_size,
			   GLint myswap_bytes);

static int checkMipmapArgs(GLenum, GLenum, GLenum);
static GLboolean legalFormat(GLenum);
static GLboolean legalType(GLenum);
static GLboolean isTypePackedPixel(GLenum);
static GLboolean isLegalFormatForPackedPixelType(GLenum, GLenum);
static GLboolean isLegalLevels(GLint, GLint, GLint, GLint);
static void closestFit(GLenum, GLint, GLint, GLint, GLenum, GLenum,
		       GLint *, GLint *);

/* all extract/shove routines must return double to handle unsigned ints */
static GLdouble extractUbyte(int, const void *);
static void shoveUbyte(GLdouble, int, void *);
static GLdouble extractSbyte(int, const void *);
static void shoveSbyte(GLdouble, int, void *);
static GLdouble extractUshort(int, const void *);
static void shoveUshort(GLdouble, int, void *);
static GLdouble extractSshort(int, const void *);
static void shoveSshort(GLdouble, int, void *);
static GLdouble extractUint(int, const void *);
static void shoveUint(GLdouble, int, void *);
static GLdouble extractSint(int, const void *);
static void shoveSint(GLdouble, int, void *);
static GLdouble extractFloat(int, const void *);
static void shoveFloat(GLdouble, int, void *);
static void halveImageSlice(int, GLdouble (*)(int, const void *),
			    void (*)(GLdouble, int, void *),
			    GLint, GLint, GLint,
			    const void *, void *,
			    GLint, GLint, GLint, GLint, GLint);
static void halveImage3D(int, GLdouble (*)(int, const void *),
			 void (*)(GLdouble, int, void *),
			 GLint, GLint, GLint,
			 const void *, void *,
			 GLint, GLint, GLint, GLint, GLint);

/* packedpixel type scale routines */
static void extract332(int,const void *, GLfloat []);
static void shove332(const GLfloat [],int ,void *);
static void extract233rev(int,const void *, GLfloat []);
static void shove233rev(const GLfloat [],int ,void *);
static void extract565(int,const void *, GLfloat []);
static void shove565(const GLfloat [],int ,void *);
static void extract565rev(int,const void *, GLfloat []);
static void shove565rev(const GLfloat [],int ,void *);
static void extract4444(int,const void *, GLfloat []);
static void shove4444(const GLfloat [],int ,void *);
static void extract4444rev(int,const void *, GLfloat []);
static void shove4444rev(const GLfloat [],int ,void *);
static void extract5551(int,const void *, GLfloat []);
static void shove5551(const GLfloat [],int ,void *);
static void extract1555rev(int,const void *, GLfloat []);
static void shove1555rev(const GLfloat [],int ,void *);
static void extract8888(int,const void *, GLfloat []);
static void shove8888(const GLfloat [],int ,void *);
static void extract8888rev(int,const void *, GLfloat []);
static void shove8888rev(const GLfloat [],int ,void *);
static void extract1010102(int,const void *, GLfloat []);
static void shove1010102(const GLfloat [],int ,void *);
static void extract2101010rev(int,const void *, GLfloat []);
static void shove2101010rev(const GLfloat [],int ,void *);
static void scaleInternalPackedPixel(int,
				     void (*)(int, const void *,GLfloat []),
				     void (*)(const GLfloat [],int, void *),
				     GLint,GLint, const void *,
				     GLint,GLint,void *,GLint,GLint,GLint);
static void halveImagePackedPixel(int,
				  void (*)(int, const void *,GLfloat []),
				  void (*)(const GLfloat [],int, void *),
				  GLint, GLint, const void *,
				  void *, GLint, GLint, GLint);
static void halve1DimagePackedPixel(int,
				    void (*)(int, const void *,GLfloat []),
				    void (*)(const GLfloat [],int, void *),
				    GLint, GLint, const void *,
				    void *, GLint, GLint, GLint);

static void halve1Dimage_ubyte(GLint, GLuint, GLuint,const GLubyte *,
			       GLubyte *, GLint, GLint, GLint);
static void halve1Dimage_byte(GLint, GLuint, GLuint,const GLbyte *, GLbyte *,
			      GLint, GLint, GLint);
static void halve1Dimage_ushort(GLint, GLuint, GLuint, const GLushort *,
				GLushort *, GLint, GLint, GLint, GLint);
static void halve1Dimage_short(GLint, GLuint, GLuint,const GLshort *, GLshort *,
			       GLint, GLint, GLint, GLint);
static void halve1Dimage_uint(GLint, GLuint, GLuint, const GLuint *, GLuint *,
			      GLint, GLint, GLint, GLint);
static void halve1Dimage_int(GLint, GLuint, GLuint, const GLint *, GLint *,
			     GLint, GLint, GLint, GLint);
static void halve1Dimage_float(GLint, GLuint, GLuint, const GLfloat *, GLfloat *,
			       GLint, GLint, GLint, GLint);

static GLint imageSize3D(GLint, GLint, GLint, GLenum,GLenum);
static void fillImage3D(const PixelStorageModes *, GLint, GLint, GLint,GLenum,
			GLenum, GLboolean, const void *, GLushort *);
static void emptyImage3D(const PixelStorageModes *,
			 GLint, GLint, GLint, GLenum,
			 GLenum, GLboolean,
			 const GLushort *, void *);
static void scaleInternal3D(GLint, GLint, GLint, GLint, const GLushort *,
			    GLint, GLint, GLint, GLushort *);

static void retrieveStoreModes(PixelStorageModes *psm)
{
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &psm->unpack_alignment);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &psm->unpack_row_length);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &psm->unpack_skip_rows);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &psm->unpack_skip_pixels);
    glGetIntegerv(GL_UNPACK_LSB_FIRST, &psm->unpack_lsb_first);
    glGetIntegerv(GL_UNPACK_SWAP_BYTES, &psm->unpack_swap_bytes);

    glGetIntegerv(GL_PACK_ALIGNMENT, &psm->pack_alignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &psm->pack_row_length);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &psm->pack_skip_rows);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &psm->pack_skip_pixels);
    glGetIntegerv(GL_PACK_LSB_FIRST, &psm->pack_lsb_first);
    glGetIntegerv(GL_PACK_SWAP_BYTES, &psm->pack_swap_bytes);
}

static void retrieveStoreModes3D(PixelStorageModes *psm)
{
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &psm->unpack_alignment);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &psm->unpack_row_length);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &psm->unpack_skip_rows);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &psm->unpack_skip_pixels);
    glGetIntegerv(GL_UNPACK_LSB_FIRST, &psm->unpack_lsb_first);
    glGetIntegerv(GL_UNPACK_SWAP_BYTES, &psm->unpack_swap_bytes);
    glGetIntegerv(GL_UNPACK_SKIP_IMAGES, &psm->unpack_skip_images);
    glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &psm->unpack_image_height);

    glGetIntegerv(GL_PACK_ALIGNMENT, &psm->pack_alignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &psm->pack_row_length);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &psm->pack_skip_rows);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &psm->pack_skip_pixels);
    glGetIntegerv(GL_PACK_LSB_FIRST, &psm->pack_lsb_first);
    glGetIntegerv(GL_PACK_SWAP_BYTES, &psm->pack_swap_bytes);
    glGetIntegerv(GL_PACK_SKIP_IMAGES, &psm->pack_skip_images);
    glGetIntegerv(GL_PACK_IMAGE_HEIGHT, &psm->pack_image_height);
}

static int computeLog(GLuint value)
{
    int i;

    i = 0;

    /* Error! */
    if (value == 0) return -1;

    for (;;) {
	if (value & 1) {
	    /* Error ! */
	    if (value != 1) return -1;
	    return i;
	}
	value = value >> 1;
	i++;
    }
}

/*
** Compute the nearest power of 2 number.  This algorithm is a little
** strange, but it works quite well.
*/
static int nearestPower(GLuint value)
{
    int i;

    i = 1;

    /* Error! */
    if (value == 0) return -1;

    for (;;) {
	if (value == 1) {
	    return i;
	} else if (value == 3) {
	    return i*4;
	}
	value = value >> 1;
	i *= 2;
    }
}

#define __GLU_SWAP_2_BYTES(s)\
(GLushort)(((GLushort)((const GLubyte*)(s))[1])<<8 | ((const GLubyte*)(s))[0])

#define __GLU_SWAP_4_BYTES(s)\
(GLuint)(((GLuint)((const GLubyte*)(s))[3])<<24 | \
        ((GLuint)((const GLubyte*)(s))[2])<<16 | \
        ((GLuint)((const GLubyte*)(s))[1])<<8  | ((const GLubyte*)(s))[0])

static void halveImage(GLint components, GLuint width, GLuint height,
		       const GLushort *datain, GLushort *dataout)
{
    int i, j, k;
    int newwidth, newheight;
    int delta;
    GLushort *s;
    const GLushort *t;

    newwidth = width / 2;
    newheight = height / 2;
    delta = width * components;
    s = dataout;
    t = datain;

    /* Piece o' cake! */
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		s[0] = (t[0] + t[components] + t[delta] +
			t[delta+components] + 2) / 4;
		s++; t++;
	    }
	    t += components;
	}
	t += delta;
    }
}

static void halveImage_ubyte(GLint components, GLuint width, GLuint height,
			const GLubyte *datain, GLubyte *dataout,
			GLint element_size, GLint ysize, GLint group_size)
{
    int i, j, k;
    int newwidth, newheight;
    int padBytes;
    GLubyte *s;
    const char *t;

    /* handle case where there is only 1 column/row */
    if (width == 1 || height == 1) {
       assert( !(width == 1 && height == 1) ); /* can't be 1x1 */
       halve1Dimage_ubyte(components,width,height,datain,dataout,
			  element_size,ysize,group_size);
       return;
    }

    newwidth = width / 2;
    newheight = height / 2;
    padBytes = ysize - (width*group_size);
    s = dataout;
    t = (const char *)datain;

    /* Piece o' cake! */
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		s[0] = (*(const GLubyte*)t +
			*(const GLubyte*)(t+group_size) +
			*(const GLubyte*)(t+ysize) +
			*(const GLubyte*)(t+ysize+group_size) + 2) / 4;
		s++; t += element_size;
	    }
	    t += group_size;
	}
	t += padBytes;
	t += ysize;
    }
}

/* */
static void halve1Dimage_ubyte(GLint components, GLuint width, GLuint height,
			       const GLubyte *dataIn, GLubyte *dataOut,
			       GLint element_size, GLint ysize,
			       GLint group_size)
{
   GLint halfWidth= width / 2;
   GLint halfHeight= height / 2;
   const char *src= (const char *) dataIn;
   GLubyte *dest= dataOut;
   int jj;

   assert(width == 1 || height == 1); /* must be 1D */
   assert(width != height);	/* can't be square */

   if (height == 1) {		/* 1 row */
      assert(width != 1);	/* widthxheight can't be 1x1 */
      halfHeight= 1;

      for (jj= 0; jj< halfWidth; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
	    *dest= (*(const GLubyte*)src +
		 *(const GLubyte*)(src+group_size)) / 2;

	    src+= element_size;
	    dest++;
	 }
	 src+= group_size;	/* skip to next 2 */
      }
      {
	 int padBytes= ysize - (width*group_size);
	 src+= padBytes;	/* for assertion only */
      }
   }
   else if (width == 1) {	/* 1 column */
      int padBytes= ysize - (width * group_size);
      assert(height != 1);	/* widthxheight can't be 1x1 */
      halfWidth= 1;
      /* one vertical column with possible pad bytes per row */
      /* average two at a time */

      for (jj= 0; jj< halfHeight; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
	    *dest= (*(const GLubyte*)src + *(const GLubyte*)(src+ysize)) / 2;

	    src+= element_size;
	    dest++;
	 }
	 src+= padBytes; /* add pad bytes, if any, to get to end to row */
	 src+= ysize;
      }
   }

   assert(src == &((const char *)dataIn)[ysize*height]);
   assert((char *)dest == &((char *)dataOut)
	  [components * element_size * halfWidth * halfHeight]);
} /* halve1Dimage_ubyte() */

static void halveImage_byte(GLint components, GLuint width, GLuint height,
			const GLbyte *datain, GLbyte *dataout,
			GLint element_size,
			GLint ysize, GLint group_size)
{
    int i, j, k;
    int newwidth, newheight;
    int padBytes;
    GLbyte *s;
    const char *t;

    /* handle case where there is only 1 column/row */
    if (width == 1 || height == 1) {
       assert( !(width == 1 && height == 1) ); /* can't be 1x1 */
       halve1Dimage_byte(components,width,height,datain,dataout,
			 element_size,ysize,group_size);
       return;
    }

    newwidth = width / 2;
    newheight = height / 2;
    padBytes = ysize - (width*group_size);
    s = dataout;
    t = (const char *)datain;

    /* Piece o' cake! */
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		s[0] = (*(const GLbyte*)t +
			*(const GLbyte*)(t+group_size) +
			*(const GLbyte*)(t+ysize) +
			*(const GLbyte*)(t+ysize+group_size) + 2) / 4;
		s++; t += element_size;
	    }
	    t += group_size;
	}
	t += padBytes;
	t += ysize;
    }
}

static void halve1Dimage_byte(GLint components, GLuint width, GLuint height,
			      const GLbyte *dataIn, GLbyte *dataOut,
			      GLint element_size,GLint ysize, GLint group_size)
{
   GLint halfWidth= width / 2;
   GLint halfHeight= height / 2;
   const char *src= (const char *) dataIn;
   GLbyte *dest= dataOut;
   int jj;

   assert(width == 1 || height == 1); /* must be 1D */
   assert(width != height);	/* can't be square */

   if (height == 1) {		/* 1 row */
      assert(width != 1);	/* widthxheight can't be 1x1 */
      halfHeight= 1;

      for (jj= 0; jj< halfWidth; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
	    *dest= (*(const GLbyte*)src + *(const GLbyte*)(src+group_size)) / 2;

	    src+= element_size;
	    dest++;
	 }
	 src+= group_size;	/* skip to next 2 */
      }
      {
	 int padBytes= ysize - (width*group_size);
	 src+= padBytes;	/* for assertion only */
      }
   }
   else if (width == 1) {	/* 1 column */
      int padBytes= ysize - (width * group_size);
      assert(height != 1);	/* widthxheight can't be 1x1 */
      halfWidth= 1;
      /* one vertical column with possible pad bytes per row */
      /* average two at a time */

      for (jj= 0; jj< halfHeight; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
	    *dest= (*(const GLbyte*)src + *(const GLbyte*)(src+ysize)) / 2;

	    src+= element_size;
	    dest++;
	 }
	 src+= padBytes; /* add pad bytes, if any, to get to end to row */
	 src+= ysize;
      }

      assert(src == &((const char *)dataIn)[ysize*height]);
   }

   assert((char *)dest == &((char *)dataOut)
	  [components * element_size * halfWidth * halfHeight]);
} /* halve1Dimage_byte() */

static void halveImage_ushort(GLint components, GLuint width, GLuint height,
			const GLushort *datain, GLushort *dataout,
			GLint element_size, GLint ysize, GLint group_size,
			GLint myswap_bytes)
{
    int i, j, k;
    int newwidth, newheight;
    int padBytes;
    GLushort *s;
    const char *t;

    /* handle case where there is only 1 column/row */
    if (width == 1 || height == 1) {
       assert( !(width == 1 && height == 1) ); /* can't be 1x1 */
       halve1Dimage_ushort(components,width,height,datain,dataout,
			   element_size,ysize,group_size, myswap_bytes);
       return;
    }

    newwidth = width / 2;
    newheight = height / 2;
    padBytes = ysize - (width*group_size);
    s = dataout;
    t = (const char *)datain;

    /* Piece o' cake! */
    if (!myswap_bytes)
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		s[0] = (*(const GLushort*)t +
			*(const GLushort*)(t+group_size) +
			*(const GLushort*)(t+ysize) +
			*(const GLushort*)(t+ysize+group_size) + 2) / 4;
		s++; t += element_size;
	    }
	    t += group_size;
	}
	t += padBytes;
	t += ysize;
    }
    else
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		s[0] = (__GLU_SWAP_2_BYTES(t) +
			__GLU_SWAP_2_BYTES(t+group_size) +
			__GLU_SWAP_2_BYTES(t+ysize) +
			__GLU_SWAP_2_BYTES(t+ysize+group_size)+ 2)/4;
		s++; t += element_size;
	    }
	    t += group_size;
	}
	t += padBytes;
	t += ysize;
    }
}

static void halve1Dimage_ushort(GLint components, GLuint width, GLuint height,
				const GLushort *dataIn, GLushort *dataOut,
				GLint element_size, GLint ysize,
				GLint group_size, GLint myswap_bytes)
{
   GLint halfWidth= width / 2;
   GLint halfHeight= height / 2;
   const char *src= (const char *) dataIn;
   GLushort *dest= dataOut;
   int jj;

   assert(width == 1 || height == 1); /* must be 1D */
   assert(width != height);	/* can't be square */

   if (height == 1) {		/* 1 row */
      assert(width != 1);	/* widthxheight can't be 1x1 */
      halfHeight= 1;

      for (jj= 0; jj< halfWidth; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
#define BOX2 2
	    GLushort ushort[BOX2];
	    if (myswap_bytes) {
	       ushort[0]= __GLU_SWAP_2_BYTES(src);
	       ushort[1]= __GLU_SWAP_2_BYTES(src+group_size);
	    }
	    else {
	       ushort[0]= *(const GLushort*)src;
	       ushort[1]= *(const GLushort*)(src+group_size);
	    }

	    *dest= (ushort[0] + ushort[1]) / 2;
	    src+= element_size;
	    dest++;
	 }
	 src+= group_size;	/* skip to next 2 */
      }
      {
	 int padBytes= ysize - (width*group_size);
	 src+= padBytes;	/* for assertion only */
      }
   }
   else if (width == 1) {	/* 1 column */
      int padBytes= ysize - (width * group_size);
      assert(height != 1);	/* widthxheight can't be 1x1 */
      halfWidth= 1;
      /* one vertical column with possible pad bytes per row */
      /* average two at a time */

      for (jj= 0; jj< halfHeight; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
#define BOX2 2
	    GLushort ushort[BOX2];
	    if (myswap_bytes) {
	       ushort[0]= __GLU_SWAP_2_BYTES(src);
	       ushort[1]= __GLU_SWAP_2_BYTES(src+ysize);
	    }
	    else {
	       ushort[0]= *(const GLushort*)src;
	       ushort[1]= *(const GLushort*)(src+ysize);
	    }
	    *dest= (ushort[0] + ushort[1]) / 2;

	    src+= element_size;
	    dest++;
	 }
	 src+= padBytes; /* add pad bytes, if any, to get to end to row */
	 src+= ysize;
      }

      assert(src == &((const char *)dataIn)[ysize*height]);
   }

   assert((char *)dest == &((char *)dataOut)
	  [components * element_size * halfWidth * halfHeight]);

} /* halve1Dimage_ushort() */


static void halveImage_short(GLint components, GLuint width, GLuint height,
			const GLshort *datain, GLshort *dataout,
			GLint element_size, GLint ysize, GLint group_size,
			GLint myswap_bytes)
{
    int i, j, k;
    int newwidth, newheight;
    int padBytes;
    GLshort *s;
    const char *t;

    /* handle case where there is only 1 column/row */
    if (width == 1 || height == 1) {
       assert( !(width == 1 && height == 1) ); /* can't be 1x1 */
       halve1Dimage_short(components,width,height,datain,dataout,
			  element_size,ysize,group_size, myswap_bytes);
       return;
    }

    newwidth = width / 2;
    newheight = height / 2;
    padBytes = ysize - (width*group_size);
    s = dataout;
    t = (const char *)datain;

    /* Piece o' cake! */
    if (!myswap_bytes)
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		s[0] = (*(const GLshort*)t +
			*(const GLshort*)(t+group_size) +
			*(const GLshort*)(t+ysize) +
			*(const GLshort*)(t+ysize+group_size) + 2) / 4;
		s++; t += element_size;
	    }
	    t += group_size;
	}
	t += padBytes;
	t += ysize;
    }
    else
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		GLushort b;
		GLint buf;
		b = __GLU_SWAP_2_BYTES(t);
		buf = *(const GLshort*)&b;
		b = __GLU_SWAP_2_BYTES(t+group_size);
		buf += *(const GLshort*)&b;
		b = __GLU_SWAP_2_BYTES(t+ysize);
		buf += *(const GLshort*)&b;
		b = __GLU_SWAP_2_BYTES(t+ysize+group_size);
		buf += *(const GLshort*)&b;
		s[0] = (GLshort)((buf+2)/4);
		s++; t += element_size;
	    }
	    t += group_size;
	}
	t += padBytes;
	t += ysize;
    }
}

static void halve1Dimage_short(GLint components, GLuint width, GLuint height,
				const GLshort *dataIn, GLshort *dataOut,
				GLint element_size, GLint ysize,
				GLint group_size, GLint myswap_bytes)
{
   GLint halfWidth= width / 2;
   GLint halfHeight= height / 2;
   const char *src= (const char *) dataIn;
   GLshort *dest= dataOut;
   int jj;

   assert(width == 1 || height == 1); /* must be 1D */
   assert(width != height);	/* can't be square */

   if (height == 1) {		/* 1 row */
      assert(width != 1);	/* widthxheight can't be 1x1 */
      halfHeight= 1;

      for (jj= 0; jj< halfWidth; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
#define BOX2 2
	    GLshort sshort[BOX2];
	    if (myswap_bytes) {
	       sshort[0]= __GLU_SWAP_2_BYTES(src);
	       sshort[1]= __GLU_SWAP_2_BYTES(src+group_size);
	    }
	    else {
	       sshort[0]= *(const GLshort*)src;
	       sshort[1]= *(const GLshort*)(src+group_size);
	    }

	    *dest= (sshort[0] + sshort[1]) / 2;
	    src+= element_size;
	    dest++;
	 }
	 src+= group_size;	/* skip to next 2 */
      }
      {
	 int padBytes= ysize - (width*group_size);
	 src+= padBytes;	/* for assertion only */
      }
   }
   else if (width == 1) {	/* 1 column */
      int padBytes= ysize - (width * group_size);
      assert(height != 1);	/* widthxheight can't be 1x1 */
      halfWidth= 1;
      /* one vertical column with possible pad bytes per row */
      /* average two at a time */

      for (jj= 0; jj< halfHeight; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
#define BOX2 2
	    GLshort sshort[BOX2];
	    if (myswap_bytes) {
	       sshort[0]= __GLU_SWAP_2_BYTES(src);
	       sshort[1]= __GLU_SWAP_2_BYTES(src+ysize);
	    }
	    else {
	       sshort[0]= *(const GLshort*)src;
	       sshort[1]= *(const GLshort*)(src+ysize);
	    }
	    *dest= (sshort[0] + sshort[1]) / 2;

	    src+= element_size;
	    dest++;
	 }
	 src+= padBytes; /* add pad bytes, if any, to get to end to row */
	 src+= ysize;
      }

      assert(src == &((const char *)dataIn)[ysize*height]);
   }

   assert((char *)dest == &((char *)dataOut)
	  [components * element_size * halfWidth * halfHeight]);

} /* halve1Dimage_short() */


static void halveImage_uint(GLint components, GLuint width, GLuint height,
			const GLuint *datain, GLuint *dataout,
			GLint element_size, GLint ysize, GLint group_size,
			GLint myswap_bytes)
{
    int i, j, k;
    int newwidth, newheight;
    int padBytes;
    GLuint *s;
    const char *t;

    /* handle case where there is only 1 column/row */
    if (width == 1 || height == 1) {
       assert( !(width == 1 && height == 1) ); /* can't be 1x1 */
       halve1Dimage_uint(components,width,height,datain,dataout,
			 element_size,ysize,group_size, myswap_bytes);
       return;
    }

    newwidth = width / 2;
    newheight = height / 2;
    padBytes = ysize - (width*group_size);
    s = dataout;
    t = (const char *)datain;

    /* Piece o' cake! */
    if (!myswap_bytes)
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		/* need to cast to double to hold large unsigned ints */
		s[0] = ((double)*(const GLuint*)t +
			(double)*(const GLuint*)(t+group_size) +
			(double)*(const GLuint*)(t+ysize) +
			(double)*(const GLuint*)(t+ysize+group_size))/4 + 0.5;
		s++; t += element_size;

	    }
	    t += group_size;
	}
	t += padBytes;
	t += ysize;
    }
    else
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		/* need to cast to double to hold large unsigned ints */
		GLdouble buf;
		buf = (GLdouble)__GLU_SWAP_4_BYTES(t) +
		      (GLdouble)__GLU_SWAP_4_BYTES(t+group_size) +
		      (GLdouble)__GLU_SWAP_4_BYTES(t+ysize) +
		      (GLdouble)__GLU_SWAP_4_BYTES(t+ysize+group_size);
		s[0] = (GLuint)(buf/4 + 0.5);

		s++; t += element_size;
	    }
	    t += group_size;
	}
	t += padBytes;
	t += ysize;
    }
}

/* */
static void halve1Dimage_uint(GLint components, GLuint width, GLuint height,
			      const GLuint *dataIn, GLuint *dataOut,
			      GLint element_size, GLint ysize,
			      GLint group_size, GLint myswap_bytes)
{
   GLint halfWidth= width / 2;
   GLint halfHeight= height / 2;
   const char *src= (const char *) dataIn;
   GLuint *dest= dataOut;
   int jj;

   assert(width == 1 || height == 1); /* must be 1D */
   assert(width != height);	/* can't be square */

   if (height == 1) {		/* 1 row */
      assert(width != 1);	/* widthxheight can't be 1x1 */
      halfHeight= 1;

      for (jj= 0; jj< halfWidth; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
#define BOX2 2
	    GLuint uint[BOX2];
	    if (myswap_bytes) {
	       uint[0]= __GLU_SWAP_4_BYTES(src);
	       uint[1]= __GLU_SWAP_4_BYTES(src+group_size);
	    }
	    else {
	       uint[0]= *(const GLuint*)src;
	       uint[1]= *(const GLuint*)(src+group_size);
	    }
	    *dest= ((double)uint[0]+(double)uint[1])/2.0;

	    src+= element_size;
	    dest++;
	 }
	 src+= group_size;	/* skip to next 2 */
      }
      {
	 int padBytes= ysize - (width*group_size);
	 src+= padBytes;	/* for assertion only */
      }
   }
   else if (width == 1) {	/* 1 column */
      int padBytes= ysize - (width * group_size);
      assert(height != 1);	/* widthxheight can't be 1x1 */
      halfWidth= 1;
      /* one vertical column with possible pad bytes per row */
      /* average two at a time */

      for (jj= 0; jj< halfHeight; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
#define BOX2 2
	    GLuint uint[BOX2];
	    if (myswap_bytes) {
	       uint[0]= __GLU_SWAP_4_BYTES(src);
	       uint[1]= __GLU_SWAP_4_BYTES(src+ysize);
	    }
	    else {
	       uint[0]= *(const GLuint*)src;
	       uint[1]= *(const GLuint*)(src+ysize);
	    }
	    *dest= ((double)uint[0]+(double)uint[1])/2.0;

	    src+= element_size;
	    dest++;
	 }
	 src+= padBytes; /* add pad bytes, if any, to get to end to row */
	 src+= ysize;
      }

      assert(src == &((const char *)dataIn)[ysize*height]);
   }

   assert((char *)dest == &((char *)dataOut)
	  [components * element_size * halfWidth * halfHeight]);

} /* halve1Dimage_uint() */

static void halveImage_int(GLint components, GLuint width, GLuint height,
			const GLint *datain, GLint *dataout, GLint element_size,
			GLint ysize, GLint group_size, GLint myswap_bytes)
{
    int i, j, k;
    int newwidth, newheight;
    int padBytes;
    GLint *s;
    const char *t;

    /* handle case where there is only 1 column/row */
    if (width == 1 || height == 1) {
       assert( !(width == 1 && height == 1) ); /* can't be 1x1 */
       halve1Dimage_int(components,width,height,datain,dataout,
			element_size,ysize,group_size, myswap_bytes);
       return;
    }

    newwidth = width / 2;
    newheight = height / 2;
    padBytes = ysize - (width*group_size);
    s = dataout;
    t = (const char *)datain;

    /* Piece o' cake! */
    if (!myswap_bytes)
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		s[0] = ((float)*(const GLint*)t +
			(float)*(const GLint*)(t+group_size) +
			(float)*(const GLint*)(t+ysize) +
			(float)*(const GLint*)(t+ysize+group_size))/4 + 0.5;
		s++; t += element_size;
	    }
	    t += group_size;
	}
	t += padBytes;
	t += ysize;
    }
    else
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		GLuint b;
		GLfloat buf;
		b = __GLU_SWAP_4_BYTES(t);
		buf = *(GLint*)&b;
		b = __GLU_SWAP_4_BYTES(t+group_size);
		buf += *(GLint*)&b;
		b = __GLU_SWAP_4_BYTES(t+ysize);
		buf += *(GLint*)&b;
		b = __GLU_SWAP_4_BYTES(t+ysize+group_size);
		buf += *(GLint*)&b;
		s[0] = (GLint)(buf/4 + 0.5);

		s++; t += element_size;
	    }
	    t += group_size;
	}
	t += padBytes;
	t += ysize;
    }
}

/* */
static void halve1Dimage_int(GLint components, GLuint width, GLuint height,
			     const GLint *dataIn, GLint *dataOut,
			     GLint element_size, GLint ysize,
			     GLint group_size, GLint myswap_bytes)
{
   GLint halfWidth= width / 2;
   GLint halfHeight= height / 2;
   const char *src= (const char *) dataIn;
   GLint *dest= dataOut;
   int jj;

   assert(width == 1 || height == 1); /* must be 1D */
   assert(width != height);	/* can't be square */

   if (height == 1) {		/* 1 row */
      assert(width != 1);	/* widthxheight can't be 1x1 */
      halfHeight= 1;

      for (jj= 0; jj< halfWidth; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
#define BOX2 2
	    GLuint uint[BOX2];
	    if (myswap_bytes) {
	       uint[0]= __GLU_SWAP_4_BYTES(src);
	       uint[1]= __GLU_SWAP_4_BYTES(src+group_size);
	    }
	    else {
	       uint[0]= *(const GLuint*)src;
	       uint[1]= *(const GLuint*)(src+group_size);
	    }
	    *dest= ((float)uint[0]+(float)uint[1])/2.0;

	    src+= element_size;
	    dest++;
	 }
	 src+= group_size;	/* skip to next 2 */
      }
      {
	 int padBytes= ysize - (width*group_size);
	 src+= padBytes;	/* for assertion only */
      }
   }
   else if (width == 1) {	/* 1 column */
      int padBytes= ysize - (width * group_size);
      assert(height != 1);	/* widthxheight can't be 1x1 */
      halfWidth= 1;
      /* one vertical column with possible pad bytes per row */
      /* average two at a time */

      for (jj= 0; jj< halfHeight; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
#define BOX2 2
	    GLuint uint[BOX2];
	    if (myswap_bytes) {
	       uint[0]= __GLU_SWAP_4_BYTES(src);
	       uint[1]= __GLU_SWAP_4_BYTES(src+ysize);
	    }
	    else {
	       uint[0]= *(const GLuint*)src;
	       uint[1]= *(const GLuint*)(src+ysize);
	    }
	    *dest= ((float)uint[0]+(float)uint[1])/2.0;

	    src+= element_size;
	    dest++;
	 }
	 src+= padBytes; /* add pad bytes, if any, to get to end to row */
	 src+= ysize;
      }

      assert(src == &((const char *)dataIn)[ysize*height]);
   }

   assert((char *)dest == &((char *)dataOut)
	  [components * element_size * halfWidth * halfHeight]);

} /* halve1Dimage_int() */


static void halveImage_float(GLint components, GLuint width, GLuint height,
			const GLfloat *datain, GLfloat *dataout,
			GLint element_size, GLint ysize, GLint group_size,
			GLint myswap_bytes)
{
    int i, j, k;
    int newwidth, newheight;
    int padBytes;
    GLfloat *s;
    const char *t;

    /* handle case where there is only 1 column/row */
    if (width == 1 || height == 1) {
       assert( !(width == 1 && height == 1) ); /* can't be 1x1 */
       halve1Dimage_float(components,width,height,datain,dataout,
			  element_size,ysize,group_size, myswap_bytes);
       return;
    }

    newwidth = width / 2;
    newheight = height / 2;
    padBytes = ysize - (width*group_size);
    s = dataout;
    t = (const char *)datain;

    /* Piece o' cake! */
    if (!myswap_bytes)
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		s[0] = (*(const GLfloat*)t +
			*(const GLfloat*)(t+group_size) +
			*(const GLfloat*)(t+ysize) +
			*(const GLfloat*)(t+ysize+group_size)) / 4;
		s++; t += element_size;
	    }
	    t += group_size;
	}
	t += padBytes;
	t += ysize;
    }
    else
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		union { GLuint b; GLfloat f; } swapbuf;
		swapbuf.b = __GLU_SWAP_4_BYTES(t);
		s[0] = swapbuf.f;
		swapbuf.b = __GLU_SWAP_4_BYTES(t+group_size);
		s[0] += swapbuf.f;
		swapbuf.b = __GLU_SWAP_4_BYTES(t+ysize);
		s[0] += swapbuf.f;
		swapbuf.b = __GLU_SWAP_4_BYTES(t+ysize+group_size);
		s[0] += swapbuf.f;
		s[0] /= 4;
		s++; t += element_size;
	    }
	    t += group_size;
	}
	t += padBytes;
	t += ysize;
    }
}

/* */
static void halve1Dimage_float(GLint components, GLuint width, GLuint height,
			       const GLfloat *dataIn, GLfloat *dataOut,
			       GLint element_size, GLint ysize,
			       GLint group_size, GLint myswap_bytes)
{
   GLint halfWidth= width / 2;
   GLint halfHeight= height / 2;
   const char *src= (const char *) dataIn;
   GLfloat *dest= dataOut;
   int jj;

   assert(width == 1 || height == 1); /* must be 1D */
   assert(width != height);	/* can't be square */

   if (height == 1) {		/* 1 row */
      assert(width != 1);	/* widthxheight can't be 1x1 */
      halfHeight= 1;

      for (jj= 0; jj< halfWidth; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
#define BOX2 2
	    GLfloat sfloat[BOX2];
	    if (myswap_bytes) {
	       sfloat[0]= __GLU_SWAP_4_BYTES(src);
	       sfloat[1]= __GLU_SWAP_4_BYTES(src+group_size);
	    }
	    else {
	       sfloat[0]= *(const GLfloat*)src;
	       sfloat[1]= *(const GLfloat*)(src+group_size);
	    }

	    *dest= (sfloat[0] + sfloat[1]) / 2.0;
	    src+= element_size;
	    dest++;
	 }
	 src+= group_size;	/* skip to next 2 */
      }
      {
	 int padBytes= ysize - (width*group_size);
	 src+= padBytes;	/* for assertion only */
      }
   }
   else if (width == 1) {	/* 1 column */
      int padBytes= ysize - (width * group_size);
      assert(height != 1);	/* widthxheight can't be 1x1 */
      halfWidth= 1;
      /* one vertical column with possible pad bytes per row */
      /* average two at a time */

      for (jj= 0; jj< halfHeight; jj++) {
	 int kk;
	 for (kk= 0; kk< components; kk++) {
#define BOX2 2
	    GLfloat sfloat[BOX2];
	    if (myswap_bytes) {
	       sfloat[0]= __GLU_SWAP_4_BYTES(src);
	       sfloat[1]= __GLU_SWAP_4_BYTES(src+ysize);
	    }
	    else {
	       sfloat[0]= *(const GLfloat*)src;
	       sfloat[1]= *(const GLfloat*)(src+ysize);
	    }
	    *dest= (sfloat[0] + sfloat[1]) / 2.0;

	    src+= element_size;
	    dest++;
	 }
	 src+= padBytes; /* add pad bytes, if any, to get to end to row */
	 src+= ysize;		/* skip to odd row */
      }
   }

   assert(src == &((const char *)dataIn)[ysize*height]);
   assert((char *)dest == &((char *)dataOut)
	  [components * element_size * halfWidth * halfHeight]);
} /* halve1Dimage_float() */

static void scale_internal(GLint components, GLint widthin, GLint heightin,
			   const GLushort *datain,
			   GLint widthout, GLint heightout,
			   GLushort *dataout)
{
    float x, lowx, highx, convx, halfconvx;
    float y, lowy, highy, convy, halfconvy;
    float xpercent,ypercent;
    float percent;
    /* Max components in a format is 4, so... */
    float totals[4];
    float area;
    int i,j,k,yint,xint,xindex,yindex;
    int temp;

    if (widthin == widthout*2 && heightin == heightout*2) {
	halveImage(components, widthin, heightin, datain, dataout);
	return;
    }
    convy = (float) heightin/heightout;
    convx = (float) widthin/widthout;
    halfconvx = convx/2;
    halfconvy = convy/2;
    for (i = 0; i < heightout; i++) {
	y = convy * (i+0.5);
	if (heightin > heightout) {
	    highy = y + halfconvy;
	    lowy = y - halfconvy;
	} else {
	    highy = y + 0.5;
	    lowy = y - 0.5;
	}
	for (j = 0; j < widthout; j++) {
	    x = convx * (j+0.5);
	    if (widthin > widthout) {
		highx = x + halfconvx;
		lowx = x - halfconvx;
	    } else {
		highx = x + 0.5;
		lowx = x - 0.5;
	    }

	    /*
	    ** Ok, now apply box filter to box that goes from (lowx, lowy)
	    ** to (highx, highy) on input data into this pixel on output
	    ** data.
	    */
	    totals[0] = totals[1] = totals[2] = totals[3] = 0.0;
	    area = 0.0;

	    y = lowy;
	    yint = floor(y);
	    while (y < highy) {
		yindex = (yint + heightin) % heightin;
		if (highy < yint+1) {
		    ypercent = highy - y;
		} else {
		    ypercent = yint+1 - y;
		}

		x = lowx;
		xint = floor(x);

		while (x < highx) {
		    xindex = (xint + widthin) % widthin;
		    if (highx < xint+1) {
			xpercent = highx - x;
		    } else {
			xpercent = xint+1 - x;
		    }

		    percent = xpercent * ypercent;
		    area += percent;
		    temp = (xindex + (yindex * widthin)) * components;
		    for (k = 0; k < components; k++) {
			totals[k] += datain[temp + k] * percent;
		    }

		    xint++;
		    x = xint;
		}
		yint++;
		y = yint;
	    }

	    temp = (j + (i * widthout)) * components;
	    for (k = 0; k < components; k++) {
		/* totals[] should be rounded in the case of enlarging an RGB
		 * ramp when the type is 332 or 4444
		 */
		dataout[temp + k] = (totals[k]+0.5)/area;
	    }
	}
    }
}

static void scale_internal_ubyte(GLint components, GLint widthin,
			   GLint heightin, const GLubyte *datain,
			   GLint widthout, GLint heightout,
			   GLubyte *dataout, GLint element_size,
			   GLint ysize, GLint group_size)
{
    float convx;
    float convy;
    float percent;
    /* Max components in a format is 4, so... */
    float totals[4];
    float area;
    int i,j,k,xindex;

    const char *temp, *temp0;
    const char *temp_index;
    int outindex;

    int lowx_int, highx_int, lowy_int, highy_int;
    float x_percent, y_percent;
    float lowx_float, highx_float, lowy_float, highy_float;
    float convy_float, convx_float;
    int convy_int, convx_int;
    int l, m;
    const char *left, *right;

    if (widthin == widthout*2 && heightin == heightout*2) {
	halveImage_ubyte(components, widthin, heightin,
	(const GLubyte *)datain, (GLubyte *)dataout,
	element_size, ysize, group_size);
	return;
    }
    convy = (float) heightin/heightout;
    convx = (float) widthin/widthout;
    convy_int = floor(convy);
    convy_float = convy - convy_int;
    convx_int = floor(convx);
    convx_float = convx - convx_int;

    area = convx * convy;

    lowy_int = 0;
    lowy_float = 0;
    highy_int = convy_int;
    highy_float = convy_float;

    for (i = 0; i < heightout; i++) {
        /* Clamp here to be sure we don't read beyond input buffer. */
        if (highy_int >= heightin)
            highy_int = heightin - 1;
	lowx_int = 0;
	lowx_float = 0;
	highx_int = convx_int;
	highx_float = convx_float;

	for (j = 0; j < widthout; j++) {

	    /*
	    ** Ok, now apply box filter to box that goes from (lowx, lowy)
	    ** to (highx, highy) on input data into this pixel on output
	    ** data.
	    */
	    totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

	    /* calculate the value for pixels in the 1st row */
	    xindex = lowx_int*group_size;
	    if((highy_int>lowy_int) && (highx_int>lowx_int)) {

		y_percent = 1-lowy_float;
		temp = (const char *)datain + xindex + lowy_int * ysize;
		percent = y_percent * (1-lowx_float);
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index)) * percent;
		}
		left = temp;
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index)) * y_percent;
		    }
		}
		temp += group_size;
		right = temp;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index)) * percent;
		}

		/* calculate the value for pixels in the last row */
		y_percent = highy_float;
		percent = y_percent * (1-lowx_float);
		temp = (const char *)datain + xindex + highy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index)) * percent;
		}
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index)) * y_percent;
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index)) * percent;
		}


		/* calculate the value for pixels in the 1st and last column */
		for(m = lowy_int+1; m < highy_int; m++) {
		    left += ysize;
		    right += ysize;
		    for (k = 0; k < components;
			 k++, left += element_size, right += element_size) {
			totals[k] += (GLubyte)(*(left))*(1-lowx_float)
				+(GLubyte)(*(right))*highx_float;
		    }
		}
	    } else if (highy_int > lowy_int) {
		x_percent = highx_float - lowx_float;
		percent = (1-lowy_float)*x_percent;
		temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index)) * percent;
		}
		for(m = lowy_int+1; m < highy_int; m++) {
		    temp += ysize;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index)) * x_percent;
		    }
		}
		percent = x_percent * highy_float;
		temp += ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index)) * percent;
		}
	    } else if (highx_int > lowx_int) {
		y_percent = highy_float - lowy_float;
		percent = (1-lowx_float)*y_percent;
		temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index)) * percent;
		}
		for (l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index)) * y_percent;
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index)) * percent;
		}
	    } else {
		percent = (highy_float-lowy_float)*(highx_float-lowx_float);
		temp = (const char *)datain + xindex + lowy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index)) * percent;
		}
	    }



	    /* this is for the pixels in the body */
	    temp0 = (const char *)datain + xindex + group_size +
		 (lowy_int+1)*ysize;
	    for (m = lowy_int+1; m < highy_int; m++) {
		temp = temp0;
		for(l = lowx_int+1; l < highx_int; l++) {
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			totals[k] += (GLubyte)(*(temp_index));
		    }
		    temp += group_size;
		}
		temp0 += ysize;
	    }

	    outindex = (j + (i * widthout)) * components;
	    for (k = 0; k < components; k++) {
		dataout[outindex + k] = totals[k]/area;
		/*printf("totals[%d] = %f\n", k, totals[k]);*/
	    }
	    lowx_int = highx_int;
	    lowx_float = highx_float;
	    highx_int += convx_int;
	    highx_float += convx_float;
	    if(highx_float > 1) {
		highx_float -= 1.0;
		highx_int++;
	    }
	}
	lowy_int = highy_int;
	lowy_float = highy_float;
	highy_int += convy_int;
	highy_float += convy_float;
	if(highy_float > 1) {
	    highy_float -= 1.0;
	    highy_int++;
	}
    }
}

static void scale_internal_byte(GLint components, GLint widthin,
			   GLint heightin, const GLbyte *datain,
			   GLint widthout, GLint heightout,
			   GLbyte *dataout, GLint element_size,
			   GLint ysize, GLint group_size)
{
    float convx;
    float convy;
    float percent;
    /* Max components in a format is 4, so... */
    float totals[4];
    float area;
    int i,j,k,xindex;

    const char *temp, *temp0;
    const char *temp_index;
    int outindex;

    int lowx_int, highx_int, lowy_int, highy_int;
    float x_percent, y_percent;
    float lowx_float, highx_float, lowy_float, highy_float;
    float convy_float, convx_float;
    int convy_int, convx_int;
    int l, m;
    const char *left, *right;

    if (widthin == widthout*2 && heightin == heightout*2) {
	halveImage_byte(components, widthin, heightin,
	(const GLbyte *)datain, (GLbyte *)dataout,
	element_size, ysize, group_size);
	return;
    }
    convy = (float) heightin/heightout;
    convx = (float) widthin/widthout;
    convy_int = floor(convy);
    convy_float = convy - convy_int;
    convx_int = floor(convx);
    convx_float = convx - convx_int;

    area = convx * convy;

    lowy_int = 0;
    lowy_float = 0;
    highy_int = convy_int;
    highy_float = convy_float;

    for (i = 0; i < heightout; i++) {
        /* Clamp here to be sure we don't read beyond input buffer. */
        if (highy_int >= heightin)
            highy_int = heightin - 1;
	lowx_int = 0;
	lowx_float = 0;
	highx_int = convx_int;
	highx_float = convx_float;

	for (j = 0; j < widthout; j++) {

	    /*
	    ** Ok, now apply box filter to box that goes from (lowx, lowy)
	    ** to (highx, highy) on input data into this pixel on output
	    ** data.
	    */
	    totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

	    /* calculate the value for pixels in the 1st row */
	    xindex = lowx_int*group_size;
	    if((highy_int>lowy_int) && (highx_int>lowx_int)) {

		y_percent = 1-lowy_float;
		temp = (const char *)datain + xindex + lowy_int * ysize;
		percent = y_percent * (1-lowx_float);
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index)) * percent;
		}
		left = temp;
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index)) * y_percent;
		    }
		}
		temp += group_size;
		right = temp;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index)) * percent;
		}

		/* calculate the value for pixels in the last row */	        
		y_percent = highy_float;
		percent = y_percent * (1-lowx_float);
		temp = (const char *)datain + xindex + highy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index)) * percent;
		}
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index)) * y_percent;
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index)) * percent;
		}


		/* calculate the value for pixels in the 1st and last column */
		for(m = lowy_int+1; m < highy_int; m++) {
		    left += ysize;
		    right += ysize;
		    for (k = 0; k < components;
			 k++, left += element_size, right += element_size) {
			totals[k] += (GLbyte)(*(left))*(1-lowx_float)
				+(GLbyte)(*(right))*highx_float;
		    }
		}
	    } else if (highy_int > lowy_int) {
		x_percent = highx_float - lowx_float;
		percent = (1-lowy_float)*x_percent;
		temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index)) * percent;
		}
		for(m = lowy_int+1; m < highy_int; m++) {
		    temp += ysize;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index)) * x_percent;
		    }
		}
		percent = x_percent * highy_float;
		temp += ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index)) * percent;
		}
	    } else if (highx_int > lowx_int) {
		y_percent = highy_float - lowy_float;
		percent = (1-lowx_float)*y_percent;
		temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index)) * percent;
		}
		for (l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index)) * y_percent;
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index)) * percent;
		}
	    } else {
		percent = (highy_float-lowy_float)*(highx_float-lowx_float);
		temp = (const char *)datain + xindex + lowy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index)) * percent;
		}
	    }



	    /* this is for the pixels in the body */
	    temp0 = (const char *)datain + xindex + group_size +
		(lowy_int+1)*ysize;
	    for (m = lowy_int+1; m < highy_int; m++) {
		temp = temp0;
		for(l = lowx_int+1; l < highx_int; l++) {
		    for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
			totals[k] += (GLbyte)(*(temp_index));
		    }
		    temp += group_size;
		}
		temp0 += ysize;
	    }

	    outindex = (j + (i * widthout)) * components;
	    for (k = 0; k < components; k++) {
		dataout[outindex + k] = totals[k]/area;
		/*printf("totals[%d] = %f\n", k, totals[k]);*/
	    }
	    lowx_int = highx_int;
	    lowx_float = highx_float;
	    highx_int += convx_int;
	    highx_float += convx_float;
	    if(highx_float > 1) {
		highx_float -= 1.0;
		highx_int++;
	    }
	}
	lowy_int = highy_int;
	lowy_float = highy_float;
	highy_int += convy_int;
	highy_float += convy_float;
	if(highy_float > 1) {
	    highy_float -= 1.0;
	    highy_int++;
	}
    }
}

static void scale_internal_ushort(GLint components, GLint widthin,
			   GLint heightin, const GLushort *datain,
			   GLint widthout, GLint heightout,
			   GLushort *dataout, GLint element_size,
			   GLint ysize, GLint group_size,
			   GLint myswap_bytes)
{
    float convx;
    float convy;
    float percent;
    /* Max components in a format is 4, so... */
    float totals[4];
    float area;
    int i,j,k,xindex;

    const char *temp, *temp0;
    const char *temp_index;
    int outindex;

    int lowx_int, highx_int, lowy_int, highy_int;
    float x_percent, y_percent;
    float lowx_float, highx_float, lowy_float, highy_float;
    float convy_float, convx_float;
    int convy_int, convx_int;
    int l, m;
    const char *left, *right;

    if (widthin == widthout*2 && heightin == heightout*2) {
	halveImage_ushort(components, widthin, heightin,
	(const GLushort *)datain, (GLushort *)dataout,
	element_size, ysize, group_size, myswap_bytes);
	return;
    }
    convy = (float) heightin/heightout;
    convx = (float) widthin/widthout;
    convy_int = floor(convy);
    convy_float = convy - convy_int;
    convx_int = floor(convx);
    convx_float = convx - convx_int;

    area = convx * convy;

    lowy_int = 0;
    lowy_float = 0;
    highy_int = convy_int;
    highy_float = convy_float;

    for (i = 0; i < heightout; i++) {
        /* Clamp here to be sure we don't read beyond input buffer. */
        if (highy_int >= heightin)
            highy_int = heightin - 1;
	lowx_int = 0;
	lowx_float = 0;
	highx_int = convx_int;
	highx_float = convx_float;

	for (j = 0; j < widthout; j++) {
	    /*
	    ** Ok, now apply box filter to box that goes from (lowx, lowy)
	    ** to (highx, highy) on input data into this pixel on output
	    ** data.
	    */
	    totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

	    /* calculate the value for pixels in the 1st row */
	    xindex = lowx_int*group_size;
	    if((highy_int>lowy_int) && (highx_int>lowx_int)) {

		y_percent = 1-lowy_float;
		temp = (const char *)datain + xindex + lowy_int * ysize;
		percent = y_percent * (1-lowx_float);
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
		left = temp;
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				 __GLU_SWAP_2_BYTES(temp_index) * y_percent;
			} else {
			    totals[k] += *(const GLushort*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		right = temp;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}

		/* calculate the value for pixels in the last row */	        
		y_percent = highy_float;
		percent = y_percent * (1-lowx_float);
		temp = (const char *)datain + xindex + highy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				 __GLU_SWAP_2_BYTES(temp_index) * y_percent;
			} else {
			    totals[k] += *(const GLushort*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}

		/* calculate the value for pixels in the 1st and last column */
		for(m = lowy_int+1; m < highy_int; m++) {
		    left += ysize;
		    right += ysize;
		    for (k = 0; k < components;
			 k++, left += element_size, right += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				__GLU_SWAP_2_BYTES(left) * (1-lowx_float) +
				__GLU_SWAP_2_BYTES(right) * highx_float;
			} else {
			    totals[k] += *(const GLushort*)left * (1-lowx_float)
				       + *(const GLushort*)right * highx_float;
			}
		    }
		}
	    } else if (highy_int > lowy_int) {
		x_percent = highx_float - lowx_float;
		percent = (1-lowy_float)*x_percent;
		temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
		for(m = lowy_int+1; m < highy_int; m++) {
		    temp += ysize;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				__GLU_SWAP_2_BYTES(temp_index) * x_percent;
			} else {
			    totals[k] += *(const GLushort*)temp_index * x_percent;
			}
		    }
		}
		percent = x_percent * highy_float;
		temp += ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
	    } else if (highx_int > lowx_int) {
		y_percent = highy_float - lowy_float;
		percent = (1-lowx_float)*y_percent;
		temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
		for (l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				__GLU_SWAP_2_BYTES(temp_index) * y_percent;
			} else {
			    totals[k] += *(const GLushort*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
	    } else {
		percent = (highy_float-lowy_float)*(highx_float-lowx_float);
		temp = (const char *)datain + xindex + lowy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
	    }

	    /* this is for the pixels in the body */
	    temp0 = (const char *)datain + xindex + group_size +
		 (lowy_int+1)*ysize;
	    for (m = lowy_int+1; m < highy_int; m++) {
		temp = temp0;
		for(l = lowx_int+1; l < highx_int; l++) {
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] += __GLU_SWAP_2_BYTES(temp_index);
			} else {
			    totals[k] += *(const GLushort*)temp_index;
			}
		    }
		    temp += group_size;
		}
		temp0 += ysize;
	    }

	    outindex = (j + (i * widthout)) * components;
	    for (k = 0; k < components; k++) {
		dataout[outindex + k] = totals[k]/area;
		/*printf("totals[%d] = %f\n", k, totals[k]);*/
	    }
	    lowx_int = highx_int;
	    lowx_float = highx_float;
	    highx_int += convx_int;
	    highx_float += convx_float;
	    if(highx_float > 1) {
		highx_float -= 1.0;
		highx_int++;
	    }
	}
	lowy_int = highy_int;
	lowy_float = highy_float;
	highy_int += convy_int;
	highy_float += convy_float;
	if(highy_float > 1) {
	    highy_float -= 1.0;
	    highy_int++;
	}
    }
}

static void scale_internal_short(GLint components, GLint widthin,
			   GLint heightin, const GLshort *datain,
			   GLint widthout, GLint heightout,
			   GLshort *dataout, GLint element_size,
			   GLint ysize, GLint group_size,
			   GLint myswap_bytes)
{
    float convx;
    float convy;
    float percent;
    /* Max components in a format is 4, so... */
    float totals[4];
    float area;
    int i,j,k,xindex;

    const char *temp, *temp0;
    const char *temp_index;
    int outindex;

    int lowx_int, highx_int, lowy_int, highy_int;
    float x_percent, y_percent;
    float lowx_float, highx_float, lowy_float, highy_float;
    float convy_float, convx_float;
    int convy_int, convx_int;
    int l, m;
    const char *left, *right;

    GLushort swapbuf;	/* unsigned buffer */

    if (widthin == widthout*2 && heightin == heightout*2) {
	halveImage_short(components, widthin, heightin,
	(const GLshort *)datain, (GLshort *)dataout,
	element_size, ysize, group_size, myswap_bytes);
	return;
    }
    convy = (float) heightin/heightout;
    convx = (float) widthin/widthout;
    convy_int = floor(convy);
    convy_float = convy - convy_int;
    convx_int = floor(convx);
    convx_float = convx - convx_int;

    area = convx * convy;

    lowy_int = 0;
    lowy_float = 0;
    highy_int = convy_int;
    highy_float = convy_float;

    for (i = 0; i < heightout; i++) {
        /* Clamp here to be sure we don't read beyond input buffer. */
        if (highy_int >= heightin)
            highy_int = heightin - 1;
	lowx_int = 0;
	lowx_float = 0;
	highx_int = convx_int;
	highx_float = convx_float;

	for (j = 0; j < widthout; j++) {
	    /*
	    ** Ok, now apply box filter to box that goes from (lowx, lowy)
	    ** to (highx, highy) on input data into this pixel on output
	    ** data.
	    */
	    totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

	    /* calculate the value for pixels in the 1st row */
	    xindex = lowx_int*group_size;
	    if((highy_int>lowy_int) && (highx_int>lowx_int)) {

		y_percent = 1-lowy_float;
		temp = (const char *)datain + xindex + lowy_int * ysize;
		percent = y_percent * (1-lowx_float);
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			totals[k] += *(const GLshort*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLshort*)temp_index * percent;
		    }
		}
		left = temp;
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			    totals[k] += *(const GLshort*)&swapbuf * y_percent;
			} else {
			    totals[k] += *(const GLshort*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		right = temp;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			totals[k] += *(const GLshort*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLshort*)temp_index * percent;
		    }
		}

		/* calculate the value for pixels in the last row */
		y_percent = highy_float;
		percent = y_percent * (1-lowx_float);
		temp = (const char *)datain + xindex + highy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			totals[k] += *(const GLshort*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLshort*)temp_index * percent;
		    }
		}
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			    totals[k] += *(const GLshort*)&swapbuf * y_percent;
			} else {
			    totals[k] += *(const GLshort*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			totals[k] += *(const GLshort*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLshort*)temp_index * percent;
		    }
		}

		/* calculate the value for pixels in the 1st and last column */
		for(m = lowy_int+1; m < highy_int; m++) {
		    left += ysize;
		    right += ysize;
		    for (k = 0; k < components;
			 k++, left += element_size, right += element_size) {
			if (myswap_bytes) {
			    swapbuf = __GLU_SWAP_2_BYTES(left);
			    totals[k] += *(const GLshort*)&swapbuf * (1-lowx_float);
			    swapbuf = __GLU_SWAP_2_BYTES(right);
			    totals[k] += *(const GLshort*)&swapbuf * highx_float;
			} else {
			    totals[k] += *(const GLshort*)left * (1-lowx_float)
				       + *(const GLshort*)right * highx_float;
			}
		    }
		}
	    } else if (highy_int > lowy_int) {
		x_percent = highx_float - lowx_float;
		percent = (1-lowy_float)*x_percent;
		temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			totals[k] += *(const GLshort*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLshort*)temp_index * percent;
		    }
		}
		for(m = lowy_int+1; m < highy_int; m++) {
		    temp += ysize;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			    totals[k] += *(const GLshort*)&swapbuf * x_percent;
			} else {
			    totals[k] += *(const GLshort*)temp_index * x_percent;
			}
		    }
		}
		percent = x_percent * highy_float;
		temp += ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			totals[k] += *(const GLshort*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLshort*)temp_index * percent;
		    }
		}
	    } else if (highx_int > lowx_int) {
		y_percent = highy_float - lowy_float;
		percent = (1-lowx_float)*y_percent;

	     temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			totals[k] += *(const GLshort*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLshort*)temp_index * percent;
		    }
		}
		for (l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			    totals[k] += *(const GLshort*)&swapbuf * y_percent;
			} else {
			    totals[k] += *(const GLshort*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			totals[k] += *(const GLshort*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLshort*)temp_index * percent;
		    }
		}
	    } else {
		percent = (highy_float-lowy_float)*(highx_float-lowx_float);
		temp = (const char *)datain + xindex + lowy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			totals[k] += *(const GLshort*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLshort*)temp_index * percent;
		    }
		}
	    }

	    /* this is for the pixels in the body */
	    temp0 = (const char *)datain + xindex + group_size +
		 (lowy_int+1)*ysize;
	    for (m = lowy_int+1; m < highy_int; m++) {
		temp = temp0;
		for(l = lowx_int+1; l < highx_int; l++) {
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf = __GLU_SWAP_2_BYTES(temp_index);
			    totals[k] += *(const GLshort*)&swapbuf;
			} else {
			    totals[k] += *(const GLshort*)temp_index;
			}
		    }
		    temp += group_size;
		}
		temp0 += ysize;
	    }

	    outindex = (j + (i * widthout)) * components;
	    for (k = 0; k < components; k++) {
		dataout[outindex + k] = totals[k]/area;
		/*printf("totals[%d] = %f\n", k, totals[k]);*/
	    }
	    lowx_int = highx_int;
	    lowx_float = highx_float;
	    highx_int += convx_int;
	    highx_float += convx_float;
	    if(highx_float > 1) {
		highx_float -= 1.0;
		highx_int++;
	    }
	}
	lowy_int = highy_int;
	lowy_float = highy_float;
	highy_int += convy_int;
	highy_float += convy_float;
	if(highy_float > 1) {
	    highy_float -= 1.0;
	    highy_int++;
	}
    }
}

static void scale_internal_uint(GLint components, GLint widthin,
			   GLint heightin, const GLuint *datain,
			   GLint widthout, GLint heightout,
			   GLuint *dataout, GLint element_size,
			   GLint ysize, GLint group_size,
			   GLint myswap_bytes)
{
    float convx;
    float convy;
    float percent;
    /* Max components in a format is 4, so... */
    float totals[4];
    float area;
    int i,j,k,xindex;

    const char *temp, *temp0;
    const char *temp_index;
    int outindex;

    int lowx_int, highx_int, lowy_int, highy_int;
    float x_percent, y_percent;
    float lowx_float, highx_float, lowy_float, highy_float;
    float convy_float, convx_float;
    int convy_int, convx_int;
    int l, m;
    const char *left, *right;

    if (widthin == widthout*2 && heightin == heightout*2) {
	halveImage_uint(components, widthin, heightin,
	(const GLuint *)datain, (GLuint *)dataout,
	element_size, ysize, group_size, myswap_bytes);
	return;
    }
    convy = (float) heightin/heightout;
    convx = (float) widthin/widthout;
    convy_int = floor(convy);
    convy_float = convy - convy_int;
    convx_int = floor(convx);
    convx_float = convx - convx_int;

    area = convx * convy;

    lowy_int = 0;
    lowy_float = 0;
    highy_int = convy_int;
    highy_float = convy_float;

    for (i = 0; i < heightout; i++) {
        /* Clamp here to be sure we don't read beyond input buffer. */
        if (highy_int >= heightin)
            highy_int = heightin - 1;
	lowx_int = 0;
	lowx_float = 0;
	highx_int = convx_int;
	highx_float = convx_float;

	for (j = 0; j < widthout; j++) {
	    /*
	    ** Ok, now apply box filter to box that goes from (lowx, lowy)
	    ** to (highx, highy) on input data into this pixel on output
	    ** data.
	    */
	    totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

	    /* calculate the value for pixels in the 1st row */
	    xindex = lowx_int*group_size;
	    if((highy_int>lowy_int) && (highx_int>lowx_int)) {

		y_percent = 1-lowy_float;
		temp = (const char *)datain + xindex + lowy_int * ysize;
		percent = y_percent * (1-lowx_float);
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_4_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLuint*)temp_index * percent;
		    }
		}
		left = temp;
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				 __GLU_SWAP_4_BYTES(temp_index) * y_percent;
			} else {
			    totals[k] += *(const GLuint*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		right = temp;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_4_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLuint*)temp_index * percent;
		    }
		}

		/* calculate the value for pixels in the last row */
		y_percent = highy_float;
		percent = y_percent * (1-lowx_float);
		temp = (const char *)datain + xindex + highy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_4_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLuint*)temp_index * percent;
		    }
		}
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				 __GLU_SWAP_4_BYTES(temp_index) * y_percent;
			} else {
			    totals[k] += *(const GLuint*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_4_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLuint*)temp_index * percent;
		    }
		}

		/* calculate the value for pixels in the 1st and last column */
		for(m = lowy_int+1; m < highy_int; m++) {
		    left += ysize;
		    right += ysize;
		    for (k = 0; k < components;
			 k++, left += element_size, right += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				__GLU_SWAP_4_BYTES(left) * (1-lowx_float)
			      + __GLU_SWAP_4_BYTES(right) * highx_float;
			} else {
			    totals[k] += *(const GLuint*)left * (1-lowx_float)
				       + *(const GLuint*)right * highx_float;
			}
		    }
		}
	    } else if (highy_int > lowy_int) {
		x_percent = highx_float - lowx_float;
		percent = (1-lowy_float)*x_percent;
		temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_4_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLuint*)temp_index * percent;
		    }
		}
		for(m = lowy_int+1; m < highy_int; m++) {
		    temp += ysize;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				 __GLU_SWAP_4_BYTES(temp_index) * x_percent;
			} else {
			    totals[k] += *(const GLuint*)temp_index * x_percent;
			}
		    }
		}
		percent = x_percent * highy_float;
		temp += ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_4_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLuint*)temp_index * percent;
		    }
		}
	    } else if (highx_int > lowx_int) {
		y_percent = highy_float - lowy_float;
		percent = (1-lowx_float)*y_percent;

	     temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_4_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLuint*)temp_index * percent;
		    }
		}
		for (l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				 __GLU_SWAP_4_BYTES(temp_index) * y_percent;
			} else {
			    totals[k] += *(const GLuint*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_4_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLuint*)temp_index * percent;
		    }
		}
	    } else {
		percent = (highy_float-lowy_float)*(highx_float-lowx_float);
		temp = (const char *)datain + xindex + lowy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_4_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLuint*)temp_index * percent;
		    }
		}
	    }

	    /* this is for the pixels in the body */
	    temp0 = (const char *)datain + xindex + group_size +
		 (lowy_int+1)*ysize;
	    for (m = lowy_int+1; m < highy_int; m++) {
		temp = temp0;
		for(l = lowx_int+1; l < highx_int; l++) {
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] += __GLU_SWAP_4_BYTES(temp_index);
			} else {
			    totals[k] += *(const GLuint*)temp_index;
			}
		    }
		    temp += group_size;
		}
		temp0 += ysize;
	    }

	    outindex = (j + (i * widthout)) * components;
	    for (k = 0; k < components; k++) {
		/* clamp at UINT_MAX */
		float value= totals[k]/area;
		if (value >= (float) UINT_MAX) {	/* need '=' */
		  dataout[outindex + k] = UINT_MAX;
		}
		else dataout[outindex + k] = value;
	    }
	    lowx_int = highx_int;
	    lowx_float = highx_float;
	    highx_int += convx_int;
	    highx_float += convx_float;
	    if(highx_float > 1) {
		highx_float -= 1.0;
		highx_int++;
	    }
	}
	lowy_int = highy_int;
	lowy_float = highy_float;
	highy_int += convy_int;
	highy_float += convy_float;
	if(highy_float > 1) {
	    highy_float -= 1.0;
	    highy_int++;
	}
    }
}



static void scale_internal_int(GLint components, GLint widthin,
			   GLint heightin, const GLint *datain,
			   GLint widthout, GLint heightout,
			   GLint *dataout, GLint element_size,
			   GLint ysize, GLint group_size,
			   GLint myswap_bytes)
{
    float convx;
    float convy;
    float percent;
    /* Max components in a format is 4, so... */
    float totals[4];
    float area;
    int i,j,k,xindex;

    const char *temp, *temp0;
    const char *temp_index;
    int outindex;

    int lowx_int, highx_int, lowy_int, highy_int;
    float x_percent, y_percent;
    float lowx_float, highx_float, lowy_float, highy_float;
    float convy_float, convx_float;
    int convy_int, convx_int;
    int l, m;
    const char *left, *right;

    GLuint swapbuf;	/* unsigned buffer */

    if (widthin == widthout*2 && heightin == heightout*2) {
	halveImage_int(components, widthin, heightin,
	(const GLint *)datain, (GLint *)dataout,
	element_size, ysize, group_size, myswap_bytes);
	return;
    }
    convy = (float) heightin/heightout;
    convx = (float) widthin/widthout;
    convy_int = floor(convy);
    convy_float = convy - convy_int;
    convx_int = floor(convx);
    convx_float = convx - convx_int;

    area = convx * convy;

    lowy_int = 0;
    lowy_float = 0;
    highy_int = convy_int;
    highy_float = convy_float;

    for (i = 0; i < heightout; i++) {
        /* Clamp here to be sure we don't read beyond input buffer. */
        if (highy_int >= heightin)
            highy_int = heightin - 1;
	lowx_int = 0;
	lowx_float = 0;
	highx_int = convx_int;
	highx_float = convx_float;

	for (j = 0; j < widthout; j++) {
	    /*
	    ** Ok, now apply box filter to box that goes from (lowx, lowy)
	    ** to (highx, highy) on input data into this pixel on output
	    ** data.
	    */
	    totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

	    /* calculate the value for pixels in the 1st row */
	    xindex = lowx_int*group_size;
	    if((highy_int>lowy_int) && (highx_int>lowx_int)) {

		y_percent = 1-lowy_float;
		temp = (const char *)datain + xindex + lowy_int * ysize;
		percent = y_percent * (1-lowx_float);
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += *(const GLint*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLint*)temp_index * percent;
		    }
		}
		left = temp;
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			    totals[k] += *(const GLint*)&swapbuf * y_percent;
			} else {
			    totals[k] += *(const GLint*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		right = temp;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += *(const GLint*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLint*)temp_index * percent;
		    }
		}

		/* calculate the value for pixels in the last row */
		y_percent = highy_float;
		percent = y_percent * (1-lowx_float);
		temp = (const char *)datain + xindex + highy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += *(const GLint*)&swapbuf  * percent;
		    } else {
			totals[k] += *(const GLint*)temp_index * percent;
		    }
		}
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			    totals[k] += *(const GLint*)&swapbuf * y_percent;
			} else {
			    totals[k] += *(const GLint*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += *(const GLint*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLint*)temp_index * percent;
		    }
		}

		/* calculate the value for pixels in the 1st and last column */
		for(m = lowy_int+1; m < highy_int; m++) {
		    left += ysize;
		    right += ysize;
		    for (k = 0; k < components;
			 k++, left += element_size, right += element_size) {
			if (myswap_bytes) {
			    swapbuf = __GLU_SWAP_4_BYTES(left);
			    totals[k] += *(const GLint*)&swapbuf * (1-lowx_float);
			    swapbuf = __GLU_SWAP_4_BYTES(right);
			    totals[k] += *(const GLint*)&swapbuf * highx_float;
			} else {
			    totals[k] += *(const GLint*)left * (1-lowx_float)
				       + *(const GLint*)right * highx_float;
			}
		    }
		}
	    } else if (highy_int > lowy_int) {
		x_percent = highx_float - lowx_float;
		percent = (1-lowy_float)*x_percent;
		temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += *(const GLint*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLint*)temp_index * percent;
		    }
		}
		for(m = lowy_int+1; m < highy_int; m++) {
		    temp += ysize;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			    totals[k] += *(const GLint*)&swapbuf * x_percent;
			} else {
			    totals[k] += *(const GLint*)temp_index * x_percent;
			}
		    }
		}
		percent = x_percent * highy_float;
		temp += ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += *(const GLint*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLint*)temp_index * percent;
		    }
		}
	    } else if (highx_int > lowx_int) {
		y_percent = highy_float - lowy_float;
		percent = (1-lowx_float)*y_percent;

		 temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += *(const GLint*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLint*)temp_index * percent;
		    }
		}
		for (l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			    totals[k] += *(const GLint*)&swapbuf * y_percent;
			} else {
			    totals[k] += *(const GLint*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += *(const GLint*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLint*)temp_index * percent;
		    }
		}
	    } else {
		percent = (highy_float-lowy_float)*(highx_float-lowx_float);
		temp = (const char *)datain + xindex + lowy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += *(const GLint*)&swapbuf * percent;
		    } else {
			totals[k] += *(const GLint*)temp_index * percent;
		    }
		}
	    }

	    /* this is for the pixels in the body */
	    temp0 = (const char *)datain + xindex + group_size +
		 (lowy_int+1)*ysize;
	    for (m = lowy_int+1; m < highy_int; m++) {
		temp = temp0;
		for(l = lowx_int+1; l < highx_int; l++) {
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf = __GLU_SWAP_4_BYTES(temp_index);
			    totals[k] += *(const GLint*)&swapbuf;
			} else {
			    totals[k] += *(const GLint*)temp_index;
			}
		    }
		    temp += group_size;
		}
		temp0 += ysize;
	    }

	    outindex = (j + (i * widthout)) * components;
	    for (k = 0; k < components; k++) {
		dataout[outindex + k] = totals[k]/area;
		/*printf("totals[%d] = %f\n", k, totals[k]);*/
	    }
	    lowx_int = highx_int;
	    lowx_float = highx_float;
	    highx_int += convx_int;
	    highx_float += convx_float;
	    if(highx_float > 1) {
		highx_float -= 1.0;
		highx_int++;
	    }
	}
	lowy_int = highy_int;
	lowy_float = highy_float;
	highy_int += convy_int;
	highy_float += convy_float;
	if(highy_float > 1) {
	    highy_float -= 1.0;
	    highy_int++;
	}
    }
}



static void scale_internal_float(GLint components, GLint widthin,
			   GLint heightin, const GLfloat *datain,
			   GLint widthout, GLint heightout,
			   GLfloat *dataout, GLint element_size,
			   GLint ysize, GLint group_size,
			   GLint myswap_bytes)
{
    float convx;
    float convy;
    float percent;
    /* Max components in a format is 4, so... */
    float totals[4];
    float area;
    int i,j,k,xindex;

    const char *temp, *temp0;
    const char *temp_index;
    int outindex;

    int lowx_int, highx_int, lowy_int, highy_int;
    float x_percent, y_percent;
    float lowx_float, highx_float, lowy_float, highy_float;
    float convy_float, convx_float;
    int convy_int, convx_int;
    int l, m;
    const char *left, *right;

    union { GLuint b; GLfloat f; } swapbuf;

    if (widthin == widthout*2 && heightin == heightout*2) {
	halveImage_float(components, widthin, heightin,
	(const GLfloat *)datain, (GLfloat *)dataout,
	element_size, ysize, group_size, myswap_bytes);
	return;
    }
    convy = (float) heightin/heightout;
    convx = (float) widthin/widthout;
    convy_int = floor(convy);
    convy_float = convy - convy_int;
    convx_int = floor(convx);
    convx_float = convx - convx_int;

    area = convx * convy;

    lowy_int = 0;
    lowy_float = 0;
    highy_int = convy_int;
    highy_float = convy_float;

    for (i = 0; i < heightout; i++) {
        /* Clamp here to be sure we don't read beyond input buffer. */
        if (highy_int >= heightin)
            highy_int = heightin - 1;
	lowx_int = 0;
	lowx_float = 0;
	highx_int = convx_int;
	highx_float = convx_float;

	for (j = 0; j < widthout; j++) {
	    /*
	    ** Ok, now apply box filter to box that goes from (lowx, lowy)
	    ** to (highx, highy) on input data into this pixel on output
	    ** data.
	    */
	    totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

	    /* calculate the value for pixels in the 1st row */
	    xindex = lowx_int*group_size;
	    if((highy_int>lowy_int) && (highx_int>lowx_int)) {

		y_percent = 1-lowy_float;
		temp = (const char *)datain + xindex + lowy_int * ysize;
		percent = y_percent * (1-lowx_float);
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += swapbuf.f * percent;
		    } else {
			totals[k] += *(const GLfloat*)temp_index * percent;
		    }
		}
		left = temp;
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			    totals[k] += swapbuf.f * y_percent;
			} else {
			    totals[k] += *(const GLfloat*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		right = temp;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += swapbuf.f * percent;
		    } else {
			totals[k] += *(const GLfloat*)temp_index * percent;
		    }
		}

		/* calculate the value for pixels in the last row */
		y_percent = highy_float;
		percent = y_percent * (1-lowx_float);
		temp = (const char *)datain + xindex + highy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += swapbuf.f * percent;
		    } else {
			totals[k] += *(const GLfloat*)temp_index * percent;
		    }
		}
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			    totals[k] += swapbuf.f * y_percent;
			} else {
			    totals[k] += *(const GLfloat*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += swapbuf.f * percent;
		    } else {
			totals[k] += *(const GLfloat*)temp_index * percent;
		    }
		}

		/* calculate the value for pixels in the 1st and last column */
		for(m = lowy_int+1; m < highy_int; m++) {
		    left += ysize;
		    right += ysize;
		    for (k = 0; k < components;
			 k++, left += element_size, right += element_size) {
			if (myswap_bytes) {
			    swapbuf.b = __GLU_SWAP_4_BYTES(left);
			    totals[k] += swapbuf.f * (1-lowx_float);
			    swapbuf.b = __GLU_SWAP_4_BYTES(right);
			    totals[k] += swapbuf.f * highx_float;
			} else {
			    totals[k] += *(const GLfloat*)left * (1-lowx_float)
				       + *(const GLfloat*)right * highx_float;
			}
		    }
		}
	    } else if (highy_int > lowy_int) {
		x_percent = highx_float - lowx_float;
		percent = (1-lowy_float)*x_percent;
		temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += swapbuf.f * percent;
		    } else {
			totals[k] += *(const GLfloat*)temp_index * percent;
		    }
		}
		for(m = lowy_int+1; m < highy_int; m++) {
		    temp += ysize;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			    totals[k] += swapbuf.f * x_percent;
			} else {
			    totals[k] += *(const GLfloat*)temp_index * x_percent;
			}
		    }
		}
		percent = x_percent * highy_float;
		temp += ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += swapbuf.f * percent;
		    } else {
			totals[k] += *(const GLfloat*)temp_index * percent;
		    }
		}
	    } else if (highx_int > lowx_int) {
		y_percent = highy_float - lowy_float;
		percent = (1-lowx_float)*y_percent;

	     temp = (const char *)datain + xindex + lowy_int*ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += swapbuf.f * percent;
		    } else {
			totals[k] += *(const GLfloat*)temp_index * percent;
		    }
		}
		for (l = lowx_int+1; l < highx_int; l++) {
		    temp += group_size;
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			    totals[k] += swapbuf.f * y_percent;
			} else {
			    totals[k] += *(const GLfloat*)temp_index * y_percent;
			}
		    }
		}
		temp += group_size;
		percent = y_percent * highx_float;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += swapbuf.f * percent;
		    } else {
			totals[k] += *(const GLfloat*)temp_index * percent;
		    }
		}
	    } else {
		percent = (highy_float-lowy_float)*(highx_float-lowx_float);
		temp = (const char *)datain + xindex + lowy_int * ysize;
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			totals[k] += swapbuf.f * percent;
		    } else {
			totals[k] += *(const GLfloat*)temp_index * percent;
		    }
		}
	    }

	    /* this is for the pixels in the body */
	    temp0 = (const char *)datain + xindex + group_size +
		 (lowy_int+1)*ysize;
	    for (m = lowy_int+1; m < highy_int; m++) {
		temp = temp0;
		for(l = lowx_int+1; l < highx_int; l++) {
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    swapbuf.b = __GLU_SWAP_4_BYTES(temp_index);
			    totals[k] += swapbuf.f;
			} else {
			    totals[k] += *(const GLfloat*)temp_index;
			}
		    }
		    temp += group_size;
		}
		temp0 += ysize;
	    }

	    outindex = (j + (i * widthout)) * components;
	    for (k = 0; k < components; k++) {
		dataout[outindex + k] = totals[k]/area;
		/*printf("totals[%d] = %f\n", k, totals[k]);*/
	    }
	    lowx_int = highx_int;
	    lowx_float = highx_float;
	    highx_int += convx_int;
	    highx_float += convx_float;
	    if(highx_float > 1) {
		highx_float -= 1.0;
		highx_int++;
	    }
	}
	lowy_int = highy_int;
	lowy_float = highy_float;
	highy_int += convy_int;
	highy_float += convy_float;
	if(highy_float > 1) {
	    highy_float -= 1.0;
	    highy_int++;
	}
    }
}

static int checkMipmapArgs(GLenum internalFormat, GLenum format, GLenum type)
{
    if (!legalFormat(format) || !legalType(type)) {
	return GLU_INVALID_ENUM;
    }
    if (format == GL_STENCIL_INDEX) {
	return GLU_INVALID_ENUM;
    }

    if (!isLegalFormatForPackedPixelType(format, type)) {
	return GLU_INVALID_OPERATION;
    }

    return 0;
} /* checkMipmapArgs() */

static GLboolean legalFormat(GLenum format)
{
    switch(format) {
      case GL_COLOR_INDEX:
      case GL_STENCIL_INDEX:
      case GL_DEPTH_COMPONENT:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_BGR:
      case GL_BGRA:
	return GL_TRUE;
      default:
	return GL_FALSE;
    }
}


static GLboolean legalType(GLenum type)
{
    switch(type) {
      case GL_BITMAP:
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
      case GL_UNSIGNED_BYTE_3_3_2:
      case GL_UNSIGNED_BYTE_2_3_3_REV:  
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_5_6_5_REV:
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_8_8_8_8_REV:
      case GL_UNSIGNED_INT_10_10_10_2:
      case GL_UNSIGNED_INT_2_10_10_10_REV:
	 return GL_TRUE;
      default:
	return GL_FALSE;
    }
}

/* */
static GLboolean isTypePackedPixel(GLenum type)
{
   assert(legalType(type));

   if (type == GL_UNSIGNED_BYTE_3_3_2 ||
       type == GL_UNSIGNED_BYTE_2_3_3_REV ||
       type == GL_UNSIGNED_SHORT_5_6_5 ||
       type == GL_UNSIGNED_SHORT_5_6_5_REV ||
       type == GL_UNSIGNED_SHORT_4_4_4_4 ||
       type == GL_UNSIGNED_SHORT_4_4_4_4_REV ||
       type == GL_UNSIGNED_SHORT_5_5_5_1 ||
       type == GL_UNSIGNED_SHORT_1_5_5_5_REV ||
       type == GL_UNSIGNED_INT_8_8_8_8 ||
       type == GL_UNSIGNED_INT_8_8_8_8_REV ||
       type == GL_UNSIGNED_INT_10_10_10_2 ||
       type == GL_UNSIGNED_INT_2_10_10_10_REV) {
      return 1;
   }
   else return 0;
} /* isTypePackedPixel() */

/* Determines if the packed pixel type is compatible with the format */
static GLboolean isLegalFormatForPackedPixelType(GLenum format, GLenum type)
{
   /* if not a packed pixel type then return true */
   if (!isTypePackedPixel(type)) {
      return GL_TRUE;
   }

   /* 3_3_2/2_3_3_REV & 5_6_5/5_6_5_REV are only compatible with RGB */
   if ((type == GL_UNSIGNED_BYTE_3_3_2 || type == GL_UNSIGNED_BYTE_2_3_3_REV||
	type == GL_UNSIGNED_SHORT_5_6_5|| type == GL_UNSIGNED_SHORT_5_6_5_REV)
       && format != GL_RGB)
      return GL_FALSE;

   /* 4_4_4_4/4_4_4_4_REV & 5_5_5_1/1_5_5_5_REV & 8_8_8_8/8_8_8_8_REV &
    * 10_10_10_2/2_10_10_10_REV are only compatible with RGBA, BGRA & ABGR_EXT.
    */
   if ((type == GL_UNSIGNED_SHORT_4_4_4_4 ||
	type == GL_UNSIGNED_SHORT_4_4_4_4_REV ||
	type == GL_UNSIGNED_SHORT_5_5_5_1 ||
	type == GL_UNSIGNED_SHORT_1_5_5_5_REV ||
	type == GL_UNSIGNED_INT_8_8_8_8 ||
	type == GL_UNSIGNED_INT_8_8_8_8_REV ||
	type == GL_UNSIGNED_INT_10_10_10_2 ||
	type == GL_UNSIGNED_INT_2_10_10_10_REV) &&
       (format != GL_RGBA &&
	format != GL_BGRA)) {
      return GL_FALSE;
   }

   return GL_TRUE;
} /* isLegalFormatForPackedPixelType() */

static GLboolean isLegalLevels(GLint userLevel,GLint baseLevel,GLint maxLevel,
			       GLint totalLevels)
{
   if (baseLevel < 0 || baseLevel < userLevel || maxLevel < baseLevel ||
       totalLevels < maxLevel)
      return GL_FALSE;
   else return GL_TRUE;
} /* isLegalLevels() */

/* Given user requested texture size, determine if it fits. If it
 * doesn't then halve both sides and make the determination again
 * until it does fit (for IR only).
 * Note that proxy textures are not implemented in RE* even though
 * they advertise the texture extension.
 * Note that proxy textures are implemented but not according to spec in
 * IMPACT*.
 */
static void closestFit(GLenum target, GLint width, GLint height,
		       GLint internalFormat, GLenum format, GLenum type,
		       GLint *newWidth, GLint *newHeight)
{
   /* Use proxy textures if OpenGL version is >= 1.1 */
   if ( (strtod((const char *)glGetString(GL_VERSION),NULL) >= 1.1)
	) {
      GLint widthPowerOf2= nearestPower(width);
      GLint heightPowerOf2= nearestPower(height);       
      GLint proxyWidth;

      do {
	 /* compute level 1 width & height, clamping each at 1 */
	 GLint widthAtLevelOne= (widthPowerOf2 > 1) ?
				 widthPowerOf2 >> 1 :
				 widthPowerOf2;
	 GLint heightAtLevelOne= (heightPowerOf2 > 1) ?
				  heightPowerOf2 >> 1 :
				  heightPowerOf2;
	 GLenum proxyTarget;
	 assert(widthAtLevelOne > 0); assert(heightAtLevelOne > 0);

	 /* does width x height at level 1 & all their mipmaps fit? */
	 if (target == GL_TEXTURE_2D || target == GL_PROXY_TEXTURE_2D) {
	    proxyTarget = GL_PROXY_TEXTURE_2D;
	    glTexImage2D(proxyTarget, 1, /* must be non-zero */
			 internalFormat,
			 widthAtLevelOne,heightAtLevelOne,0,format,type,NULL);
	 } else
#if defined(GL_ARB_texture_cube_map)
	 if ((target == GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB) ||
	     (target == GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB) ||
	     (target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB) ||
	     (target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB) ||
	     (target == GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB) ||
	     (target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB)) {
	     proxyTarget = GL_PROXY_TEXTURE_CUBE_MAP_ARB;
	     glTexImage2D(proxyTarget, 1, /* must be non-zero */
			  internalFormat,
			  widthAtLevelOne,heightAtLevelOne,0,format,type,NULL);
	 } else
#endif /* GL_ARB_texture_cube_map */
	 {
	    assert(target == GL_TEXTURE_1D || target == GL_PROXY_TEXTURE_1D);
	    proxyTarget = GL_PROXY_TEXTURE_1D;
	    glTexImage1D(proxyTarget, 1, /* must be non-zero */
			 internalFormat,widthAtLevelOne,0,format,type,NULL);
	 }
	 glGetTexLevelParameteriv(proxyTarget, 1,GL_TEXTURE_WIDTH,&proxyWidth);
	 /* does it fit??? */
	 if (proxyWidth == 0) { /* nope, so try again with these sizes */
	    if (widthPowerOf2 == 1 && heightPowerOf2 == 1) {
	       /* An 1x1 texture couldn't fit for some reason, so
		* break out.  This should never happen. But things
		* happen.  The disadvantage with this if-statement is
		* that we will never be aware of when this happens
		* since it will silently branch out.
		*/
	       goto noProxyTextures;
	    }
	    widthPowerOf2= widthAtLevelOne;
	    heightPowerOf2= heightAtLevelOne;
	 }
	 /* else it does fit */
      } while (proxyWidth == 0);
      /* loop must terminate! */

      /* return the width & height at level 0 that fits */
      *newWidth= widthPowerOf2;
      *newHeight= heightPowerOf2;
/*printf("Proxy Textures\n");*/
   } /* if gluCheckExtension() */
   else {			/* no texture extension, so do this instead */
      GLint maxsize;

noProxyTextures:

      glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);
      /* clamp user's texture sizes to maximum sizes, if necessary */
      *newWidth = nearestPower(width);
      if (*newWidth > maxsize) *newWidth = maxsize;
      *newHeight = nearestPower(height);
      if (*newHeight > maxsize) *newHeight = maxsize;
/*printf("NO proxy textures\n");*/
   }
} /* closestFit() */

GLint GLAPIENTRY
gluScaleImage(GLenum format, GLsizei widthin, GLsizei heightin,
		    GLenum typein, const void *datain,
		    GLsizei widthout, GLsizei heightout, GLenum typeout,
		    void *dataout)
{
    int components;
    GLushort *beforeImage;
    GLushort *afterImage;
    PixelStorageModes psm;

    if (widthin == 0 || heightin == 0 || widthout == 0 || heightout == 0) {
	return 0;
    }
    if (widthin < 0 || heightin < 0 || widthout < 0 || heightout < 0) {
	return GLU_INVALID_VALUE;
    }
    if (!legalFormat(format) || !legalType(typein) || !legalType(typeout)) {
	return GLU_INVALID_ENUM;
    }
    if (!isLegalFormatForPackedPixelType(format, typein)) {
       return GLU_INVALID_OPERATION;
    }
    if (!isLegalFormatForPackedPixelType(format, typeout)) {
       return GLU_INVALID_OPERATION;
    }
    beforeImage =
	malloc(image_size(widthin, heightin, format, GL_UNSIGNED_SHORT));
    afterImage =
	malloc(image_size(widthout, heightout, format, GL_UNSIGNED_SHORT));
    if (beforeImage == NULL || afterImage == NULL) {
	free(beforeImage);
	free(afterImage);
	return GLU_OUT_OF_MEMORY;
    }

    retrieveStoreModes(&psm);
    fill_image(&psm,widthin, heightin, format, typein, is_index(format),
	    datain, beforeImage);
    components = elements_per_group(format, 0);
    scale_internal(components, widthin, heightin, beforeImage,
	    widthout, heightout, afterImage);
    empty_image(&psm,widthout, heightout, format, typeout,
	    is_index(format), afterImage, dataout);
    free((GLbyte *) beforeImage);
    free((GLbyte *) afterImage);

    return 0;
}

int gluBuild1DMipmapLevelsCore(GLenum target, GLint internalFormat,
			       GLsizei width,
			       GLsizei widthPowerOf2,
			       GLenum format, GLenum type,
			       GLint userLevel, GLint baseLevel,GLint maxLevel,
			       const void *data)
{
    GLint newwidth;
    GLint level, levels;
    GLushort *newImage;
    GLint newImage_width;
    GLushort *otherImage;
    GLushort *imageTemp;
    GLint memreq;
    GLint cmpts;
    PixelStorageModes psm;

    assert(checkMipmapArgs(internalFormat,format,type) == 0);
    assert(width >= 1);

    otherImage = NULL;

    newwidth= widthPowerOf2;
    levels = computeLog(newwidth);

    levels+= userLevel;

    retrieveStoreModes(&psm);
    newImage = (GLushort *)
	malloc(image_size(width, 1, format, GL_UNSIGNED_SHORT));
    newImage_width = width;
    if (newImage == NULL) {
	return GLU_OUT_OF_MEMORY;
    }
    fill_image(&psm,width, 1, format, type, is_index(format),
	    data, newImage);
    cmpts = elements_per_group(format,type);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    /*
    ** If swap_bytes was set, swapping occurred in fill_image.
    */
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

    for (level = userLevel; level <= levels; level++) {
	if (newImage_width == newwidth) {
	    /* Use newImage for this level */
	    if (baseLevel <= level && level <= maxLevel) {
	    glTexImage1D(target, level, internalFormat, newImage_width,
		    0, format, GL_UNSIGNED_SHORT, (void *) newImage);
	    }
	} else {
	    if (otherImage == NULL) {
		memreq = image_size(newwidth, 1, format, GL_UNSIGNED_SHORT);
		otherImage = (GLushort *) malloc(memreq);
		if (otherImage == NULL) {
		    glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
		    glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
		    glPixelStorei(GL_UNPACK_SKIP_PIXELS,psm.unpack_skip_pixels);
		    glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
		    glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
		    free(newImage);
		    return GLU_OUT_OF_MEMORY;
		}
	    }
	    scale_internal(cmpts, newImage_width, 1, newImage,
		    newwidth, 1, otherImage);
	    /* Swap newImage and otherImage */
	    imageTemp = otherImage;
	    otherImage = newImage;
	    newImage = imageTemp;

	    newImage_width = newwidth;
	    if (baseLevel <= level && level <= maxLevel) {
	    glTexImage1D(target, level, internalFormat, newImage_width,
		    0, format, GL_UNSIGNED_SHORT, (void *) newImage);
	    }
	}
	if (newwidth > 1) newwidth /= 2;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);

    free((GLbyte *) newImage);
    if (otherImage) {
	free((GLbyte *) otherImage);
    }
    return 0;
}

GLint GLAPIENTRY
gluBuild1DMipmapLevels(GLenum target, GLint internalFormat,
			     GLsizei width,
			     GLenum format, GLenum type,
			     GLint userLevel, GLint baseLevel, GLint maxLevel,
			     const void *data)
{
   int levels;

   int rc= checkMipmapArgs(internalFormat,format,type);
   if (rc != 0) return rc;

   if (width < 1) {
       return GLU_INVALID_VALUE;
   }

   levels = computeLog(width);

   levels+= userLevel;
   if (!isLegalLevels(userLevel,baseLevel,maxLevel,levels))
      return GLU_INVALID_VALUE;

   return gluBuild1DMipmapLevelsCore(target, internalFormat,
				     width,
				     width,format, type,
				     userLevel, baseLevel, maxLevel,
				     data);
} /* gluBuild1DMipmapLevels() */

GLint GLAPIENTRY
gluBuild1DMipmaps(GLenum target, GLint internalFormat, GLsizei width,
			GLenum format, GLenum type,
			const void *data)
{
   GLint widthPowerOf2;
   int levels;
   GLint dummy;

   int rc= checkMipmapArgs(internalFormat,format,type);
   if (rc != 0) return rc;

   if (width < 1) {
       return GLU_INVALID_VALUE;
   }

   closestFit(target,width,1,internalFormat,format,type,&widthPowerOf2,&dummy);
   levels = computeLog(widthPowerOf2);

   return gluBuild1DMipmapLevelsCore(target,internalFormat,
				     width,
				     widthPowerOf2,
				     format,type,0,0,levels,data);
}

static int bitmapBuild2DMipmaps(GLenum target, GLint internalFormat,
		     GLint width, GLint height, GLenum format,
		     GLenum type, const void *data)
{
    GLint newwidth, newheight;
    GLint level, levels;
    GLushort *newImage;
    GLint newImage_width;
    GLint newImage_height;
    GLushort *otherImage;
    GLushort *imageTemp;
    GLint memreq;
    GLint cmpts;
    PixelStorageModes psm;

    retrieveStoreModes(&psm);

#if 0
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);
    newwidth = nearestPower(width);
    if (newwidth > maxsize) newwidth = maxsize;
    newheight = nearestPower(height);
    if (newheight > maxsize) newheight = maxsize;
#else
    closestFit(target,width,height,internalFormat,format,type,
	       &newwidth,&newheight);
#endif
    levels = computeLog(newwidth);
    level = computeLog(newheight);
    if (level > levels) levels=level;

    otherImage = NULL;
    newImage = (GLushort *)
	malloc(image_size(width, height, format, GL_UNSIGNED_SHORT));
    newImage_width = width;
    newImage_height = height;
    if (newImage == NULL) {
	return GLU_OUT_OF_MEMORY;
    }

    fill_image(&psm,width, height, format, type, is_index(format),
	  data, newImage);

    cmpts = elements_per_group(format,type);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    /*
    ** If swap_bytes was set, swapping occurred in fill_image.
    */
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

    for (level = 0; level <= levels; level++) {
	if (newImage_width == newwidth && newImage_height == newheight) {	     /* Use newImage for this level */
	    glTexImage2D(target, level, internalFormat, newImage_width,
		    newImage_height, 0, format, GL_UNSIGNED_SHORT,
		    (void *) newImage);
	} else {
	    if (otherImage == NULL) {
		memreq =
		    image_size(newwidth, newheight, format, GL_UNSIGNED_SHORT);
		otherImage = (GLushort *) malloc(memreq);
		if (otherImage == NULL) {
		    glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
		    glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
		    glPixelStorei(GL_UNPACK_SKIP_PIXELS,psm.unpack_skip_pixels);
		    glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
		    glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
		    free(newImage);
		    return GLU_OUT_OF_MEMORY;
		}
	    }
	    scale_internal(cmpts, newImage_width, newImage_height, newImage,
		    newwidth, newheight, otherImage);
	    /* Swap newImage and otherImage */
	    imageTemp = otherImage;
	    otherImage = newImage;
	    newImage = imageTemp;

	    newImage_width = newwidth;
	    newImage_height = newheight;
	    glTexImage2D(target, level, internalFormat, newImage_width,
		    newImage_height, 0, format, GL_UNSIGNED_SHORT,
		    (void *) newImage);
	}
	if (newwidth > 1) newwidth /= 2;
	if (newheight > 1) newheight /= 2;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);

    free((GLbyte *) newImage);
    if (otherImage) {
	free((GLbyte *) otherImage);
    }
    return 0;
}

/* To make swapping images less error prone */
#define __GLU_INIT_SWAP_IMAGE void *tmpImage
#define __GLU_SWAP_IMAGE(a,b) tmpImage = a; a = b; b = tmpImage;

static int gluBuild2DMipmapLevelsCore(GLenum target, GLint internalFormat,
				      GLsizei width, GLsizei height,
				      GLsizei widthPowerOf2,
				      GLsizei heightPowerOf2,
				      GLenum format, GLenum type,
				      GLint userLevel,
				      GLint baseLevel,GLint maxLevel,
				      const void *data)
{
    GLint newwidth, newheight;
    GLint level, levels;
    const void *usersImage; /* passed from user. Don't touch! */
    void *srcImage, *dstImage; /* scratch area to build mipmapped images */
    __GLU_INIT_SWAP_IMAGE;
    GLint memreq;
    GLint cmpts;

    GLint myswap_bytes, groups_per_line, element_size, group_size;
    GLint rowsize, padding;
    PixelStorageModes psm;

    assert(checkMipmapArgs(internalFormat,format,type) == 0);
    assert(width >= 1 && height >= 1);

    if(type == GL_BITMAP) {
	return bitmapBuild2DMipmaps(target, internalFormat, width, height,
		format, type, data);
    }

    srcImage = dstImage = NULL;

    newwidth= widthPowerOf2;
    newheight= heightPowerOf2;
    levels = computeLog(newwidth);
    level = computeLog(newheight);
    if (level > levels) levels=level;

    levels+= userLevel;

    retrieveStoreModes(&psm);
    myswap_bytes = psm.unpack_swap_bytes;
    cmpts = elements_per_group(format,type);
    if (psm.unpack_row_length > 0) {
	groups_per_line = psm.unpack_row_length;
    } else {
	groups_per_line = width;
    }

    element_size = bytes_per_element(type);
    group_size = element_size * cmpts;
    if (element_size == 1) myswap_bytes = 0;

    rowsize = groups_per_line * group_size;
    padding = (rowsize % psm.unpack_alignment);
    if (padding) {
	rowsize += psm.unpack_alignment - padding;
    }
    usersImage = (const GLubyte *) data + psm.unpack_skip_rows * rowsize +
	psm.unpack_skip_pixels * group_size;

    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    level = userLevel;

    /* already power-of-two square */
    if (width == newwidth && height == newheight) {
	/* Use usersImage for level userLevel */
	if (baseLevel <= level && level <= maxLevel) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
	glTexImage2D(target, level, internalFormat, width,
		height, 0, format, type,
		usersImage);
	}
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	if(levels == 0) { /* we're done. clean up and return */
	  glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
	  glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
	  glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
	  glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
	  glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
	  return 0;
	}
	{
	   int nextWidth= newwidth/2;
	   int nextHeight= newheight/2;

	   /* clamp to 1 */
	   if (nextWidth < 1) nextWidth= 1;
	   if (nextHeight < 1) nextHeight= 1;
	memreq = image_size(nextWidth, nextHeight, format, type);
	}

	switch(type) {
	case GL_UNSIGNED_BYTE:
	  dstImage = (GLubyte *)malloc(memreq);
	  break;
	case GL_BYTE:
	  dstImage = (GLbyte *)malloc(memreq);
	  break;
	case GL_UNSIGNED_SHORT:
	  dstImage = (GLushort *)malloc(memreq);
	  break;
	case GL_SHORT:
	  dstImage = (GLshort *)malloc(memreq);
	  break;
	case GL_UNSIGNED_INT:
	  dstImage = (GLuint *)malloc(memreq);
	  break;
	case GL_INT:
	  dstImage = (GLint *)malloc(memreq);
	  break;
	case GL_FLOAT:
	  dstImage = (GLfloat *)malloc(memreq);
	  break;
	case GL_UNSIGNED_BYTE_3_3_2:
	case GL_UNSIGNED_BYTE_2_3_3_REV:
	  dstImage = (GLubyte *)malloc(memreq);
	  break;
	case GL_UNSIGNED_SHORT_5_6_5:
	case GL_UNSIGNED_SHORT_5_6_5_REV:
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	case GL_UNSIGNED_SHORT_5_5_5_1:
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	  dstImage = (GLushort *)malloc(memreq);
	  break;
	case GL_UNSIGNED_INT_8_8_8_8:
	case GL_UNSIGNED_INT_8_8_8_8_REV:
	case GL_UNSIGNED_INT_10_10_10_2:
	case GL_UNSIGNED_INT_2_10_10_10_REV:
	  dstImage = (GLuint *)malloc(memreq);  
	  break;
	default:
	  return GLU_INVALID_ENUM;
	}
	if (dstImage == NULL) {
	  glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
	  glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
	  glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
	  glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
	  glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
	  return GLU_OUT_OF_MEMORY;
	}
	else
	  switch(type) {
	  case GL_UNSIGNED_BYTE:
	    halveImage_ubyte(cmpts, width, height,
			     (const GLubyte *)usersImage, (GLubyte *)dstImage,
			     element_size, rowsize, group_size);
	    break;
	  case GL_BYTE:
	    halveImage_byte(cmpts, width, height,
			    (const GLbyte *)usersImage, (GLbyte *)dstImage,
			    element_size, rowsize, group_size);
	    break;
	  case GL_UNSIGNED_SHORT:
	    halveImage_ushort(cmpts, width, height,
			      (const GLushort *)usersImage, (GLushort *)dstImage,
			      element_size, rowsize, group_size, myswap_bytes);
	    break;
	  case GL_SHORT:
	    halveImage_short(cmpts, width, height,
			     (const GLshort *)usersImage, (GLshort *)dstImage,
			     element_size, rowsize, group_size, myswap_bytes);
	    break;
	  case GL_UNSIGNED_INT:
	    halveImage_uint(cmpts, width, height,
			    (const GLuint *)usersImage, (GLuint *)dstImage,
			    element_size, rowsize, group_size, myswap_bytes);
	    break;
	  case GL_INT:
	    halveImage_int(cmpts, width, height,
			   (const GLint *)usersImage, (GLint *)dstImage,
			   element_size, rowsize, group_size, myswap_bytes);
	    break;
	  case GL_FLOAT:
	    halveImage_float(cmpts, width, height,
			     (const GLfloat *)usersImage, (GLfloat *)dstImage,
			     element_size, rowsize, group_size, myswap_bytes);
	    break;
	  case GL_UNSIGNED_BYTE_3_3_2:
	    assert(format == GL_RGB);
	    halveImagePackedPixel(3,extract332,shove332,
				  width,height,usersImage,dstImage,
				  element_size,rowsize,myswap_bytes);
	    break;
	  case GL_UNSIGNED_BYTE_2_3_3_REV:
	    assert(format == GL_RGB);
	    halveImagePackedPixel(3,extract233rev,shove233rev,
				  width,height,usersImage,dstImage,
				  element_size,rowsize,myswap_bytes);
	    break;
	  case GL_UNSIGNED_SHORT_5_6_5:
	    halveImagePackedPixel(3,extract565,shove565,
				  width,height,usersImage,dstImage,
				  element_size,rowsize,myswap_bytes);
	    break;
	  case GL_UNSIGNED_SHORT_5_6_5_REV:
	    halveImagePackedPixel(3,extract565rev,shove565rev,
				  width,height,usersImage,dstImage,
				  element_size,rowsize,myswap_bytes);
	    break;
	  case GL_UNSIGNED_SHORT_4_4_4_4:
	    halveImagePackedPixel(4,extract4444,shove4444,
				  width,height,usersImage,dstImage,
				  element_size,rowsize,myswap_bytes);
	    break;
	  case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	    halveImagePackedPixel(4,extract4444rev,shove4444rev,
				  width,height,usersImage,dstImage,
				  element_size,rowsize,myswap_bytes);
	    break;
	  case GL_UNSIGNED_SHORT_5_5_5_1:
	    halveImagePackedPixel(4,extract5551,shove5551,
				  width,height,usersImage,dstImage,
				  element_size,rowsize,myswap_bytes);
	    break;
	  case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	    halveImagePackedPixel(4,extract1555rev,shove1555rev,
				  width,height,usersImage,dstImage,
				  element_size,rowsize,myswap_bytes);
	    break;
	  case GL_UNSIGNED_INT_8_8_8_8:
	    halveImagePackedPixel(4,extract8888,shove8888,
				  width,height,usersImage,dstImage,
				  element_size,rowsize,myswap_bytes);
	    break;
	  case GL_UNSIGNED_INT_8_8_8_8_REV:
	    halveImagePackedPixel(4,extract8888rev,shove8888rev,
				  width,height,usersImage,dstImage,
				  element_size,rowsize,myswap_bytes);
	    break;
	  case GL_UNSIGNED_INT_10_10_10_2:
	    halveImagePackedPixel(4,extract1010102,shove1010102,
				  width,height,usersImage,dstImage,
				  element_size,rowsize,myswap_bytes);
	    break;
	  case GL_UNSIGNED_INT_2_10_10_10_REV:
	    halveImagePackedPixel(4,extract2101010rev,shove2101010rev,
				  width,height,usersImage,dstImage,
				  element_size,rowsize,myswap_bytes);
	    break;
	  default:
	    assert(0);
	    break;
	  }
	newwidth = width/2;
	newheight = height/2;
	/* clamp to 1 */
	if (newwidth < 1) newwidth= 1;
	if (newheight < 1) newheight= 1;

	myswap_bytes = 0;
	rowsize = newwidth * group_size;
	memreq = image_size(newwidth, newheight, format, type);
	/* Swap srcImage and dstImage */
	__GLU_SWAP_IMAGE(srcImage,dstImage);
	switch(type) {
	case GL_UNSIGNED_BYTE:
	  dstImage = (GLubyte *)malloc(memreq);
	  break;
	case GL_BYTE:
	  dstImage = (GLbyte *)malloc(memreq);
	  break;
	case GL_UNSIGNED_SHORT:
	  dstImage = (GLushort *)malloc(memreq);
	  break;
	case GL_SHORT:
	  dstImage = (GLshort *)malloc(memreq);
	  break;
	case GL_UNSIGNED_INT:
	  dstImage = (GLuint *)malloc(memreq);
	  break;
	case GL_INT:
	  dstImage = (GLint *)malloc(memreq);
	  break;
	case GL_FLOAT:
	  dstImage = (GLfloat *)malloc(memreq);
	  break;
	case GL_UNSIGNED_BYTE_3_3_2:
	case GL_UNSIGNED_BYTE_2_3_3_REV:
	  dstImage = (GLubyte *)malloc(memreq);
	  break;
	case GL_UNSIGNED_SHORT_5_6_5:
	case GL_UNSIGNED_SHORT_5_6_5_REV:
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	case GL_UNSIGNED_SHORT_5_5_5_1:
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	  dstImage = (GLushort *)malloc(memreq);
	  break;
	case GL_UNSIGNED_INT_8_8_8_8:
	case GL_UNSIGNED_INT_8_8_8_8_REV:
	case GL_UNSIGNED_INT_10_10_10_2:
	case GL_UNSIGNED_INT_2_10_10_10_REV:
	  dstImage = (GLuint *)malloc(memreq);
	  break;
	default:
	  return GLU_INVALID_ENUM;
	}
	if (dstImage == NULL) {
	  glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
	  glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
	  glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
	  glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
	  glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
	  free(srcImage);
	  return GLU_OUT_OF_MEMORY;
	}
	/* level userLevel+1 is in srcImage; level userLevel already saved */
	level = userLevel+1;
    } else { /* user's image is *not* nice power-of-2 sized square */
	memreq = image_size(newwidth, newheight, format, type);
	switch(type) {
	    case GL_UNSIGNED_BYTE:
		dstImage = (GLubyte *)malloc(memreq);
		break;
	    case GL_BYTE:
		dstImage = (GLbyte *)malloc(memreq);
		break;
	    case GL_UNSIGNED_SHORT:
		dstImage = (GLushort *)malloc(memreq);
		break;
	    case GL_SHORT:
		dstImage = (GLshort *)malloc(memreq);
		break;
	    case GL_UNSIGNED_INT:
		dstImage = (GLuint *)malloc(memreq);
		break;
	    case GL_INT:
		dstImage = (GLint *)malloc(memreq);
		break;
	    case GL_FLOAT:
		dstImage = (GLfloat *)malloc(memreq);
		break;
	    case GL_UNSIGNED_BYTE_3_3_2:
	    case GL_UNSIGNED_BYTE_2_3_3_REV:
		dstImage = (GLubyte *)malloc(memreq);
		break;
	    case GL_UNSIGNED_SHORT_5_6_5:
	    case GL_UNSIGNED_SHORT_5_6_5_REV:
	    case GL_UNSIGNED_SHORT_4_4_4_4:
	    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	    case GL_UNSIGNED_SHORT_5_5_5_1:
	    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
		dstImage = (GLushort *)malloc(memreq);
		break;
	    case GL_UNSIGNED_INT_8_8_8_8:
	    case GL_UNSIGNED_INT_8_8_8_8_REV:
	    case GL_UNSIGNED_INT_10_10_10_2:
	    case GL_UNSIGNED_INT_2_10_10_10_REV:
		dstImage = (GLuint *)malloc(memreq);
		break;
	    default:
		return GLU_INVALID_ENUM;
	}

	if (dstImage == NULL) {
	    glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
	    glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
	    glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
	    glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
	    glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
	    return GLU_OUT_OF_MEMORY;
	}

	switch(type) {
	case GL_UNSIGNED_BYTE:
	    scale_internal_ubyte(cmpts, width, height,
				 (const GLubyte *)usersImage, newwidth, newheight,
				 (GLubyte *)dstImage, element_size,
				 rowsize, group_size);
	    break;
	case GL_BYTE:
	    scale_internal_byte(cmpts, width, height,
				(const GLbyte *)usersImage, newwidth, newheight,
				(GLbyte *)dstImage, element_size,
				rowsize, group_size);
	    break;
	case GL_UNSIGNED_SHORT:
	    scale_internal_ushort(cmpts, width, height,
				  (const GLushort *)usersImage, newwidth, newheight,
				  (GLushort *)dstImage, element_size,
				  rowsize, group_size, myswap_bytes);
	    break;
	case GL_SHORT:
	    scale_internal_short(cmpts, width, height,
				 (const GLshort *)usersImage, newwidth, newheight,
				 (GLshort *)dstImage, element_size,
				 rowsize, group_size, myswap_bytes);
	    break;
	case GL_UNSIGNED_INT:
	    scale_internal_uint(cmpts, width, height,
				(const GLuint *)usersImage, newwidth, newheight,
				(GLuint *)dstImage, element_size,
				rowsize, group_size, myswap_bytes);
	    break;
	case GL_INT:
	    scale_internal_int(cmpts, width, height,
			       (const GLint *)usersImage, newwidth, newheight,
			       (GLint *)dstImage, element_size,
			       rowsize, group_size, myswap_bytes);
	    break;
	case GL_FLOAT:
	    scale_internal_float(cmpts, width, height,
				 (const GLfloat *)usersImage, newwidth, newheight,
				 (GLfloat *)dstImage, element_size,
				 rowsize, group_size, myswap_bytes);
	    break;
	case GL_UNSIGNED_BYTE_3_3_2:
	    scaleInternalPackedPixel(3,extract332,shove332,
				     width, height,usersImage,
				     newwidth,newheight,(void *)dstImage,
				     element_size,rowsize,myswap_bytes);
	    break;
	case GL_UNSIGNED_BYTE_2_3_3_REV:
	    scaleInternalPackedPixel(3,extract233rev,shove233rev,
				     width, height,usersImage,
				     newwidth,newheight,(void *)dstImage,
				     element_size,rowsize,myswap_bytes);
	    break;
	case GL_UNSIGNED_SHORT_5_6_5:
	    scaleInternalPackedPixel(3,extract565,shove565,
				     width, height,usersImage,
				     newwidth,newheight,(void *)dstImage,
				     element_size,rowsize,myswap_bytes);
	    break;
	case GL_UNSIGNED_SHORT_5_6_5_REV:
	    scaleInternalPackedPixel(3,extract565rev,shove565rev,
				     width, height,usersImage,
				     newwidth,newheight,(void *)dstImage,
				     element_size,rowsize,myswap_bytes);
	    break;
	case GL_UNSIGNED_SHORT_4_4_4_4:
	    scaleInternalPackedPixel(4,extract4444,shove4444,
				     width, height,usersImage,
				     newwidth,newheight,(void *)dstImage,
				     element_size,rowsize,myswap_bytes);
	    break;
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	    scaleInternalPackedPixel(4,extract4444rev,shove4444rev,
				     width, height,usersImage,
				     newwidth,newheight,(void *)dstImage,
				     element_size,rowsize,myswap_bytes);
	    break;
	case GL_UNSIGNED_SHORT_5_5_5_1:
	    scaleInternalPackedPixel(4,extract5551,shove5551,
				     width, height,usersImage,
				     newwidth,newheight,(void *)dstImage,
				     element_size,rowsize,myswap_bytes);
	    break;
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	    scaleInternalPackedPixel(4,extract1555rev,shove1555rev,
				     width, height,usersImage,
				     newwidth,newheight,(void *)dstImage,
				     element_size,rowsize,myswap_bytes);
	    break;
	case GL_UNSIGNED_INT_8_8_8_8:
	    scaleInternalPackedPixel(4,extract8888,shove8888,
				     width, height,usersImage,
				     newwidth,newheight,(void *)dstImage,
				     element_size,rowsize,myswap_bytes);
	    break;
	case GL_UNSIGNED_INT_8_8_8_8_REV:
	    scaleInternalPackedPixel(4,extract8888rev,shove8888rev,
				     width, height,usersImage,
				     newwidth,newheight,(void *)dstImage,
				     element_size,rowsize,myswap_bytes);
	    break;
	case GL_UNSIGNED_INT_10_10_10_2:
	    scaleInternalPackedPixel(4,extract1010102,shove1010102,
				     width, height,usersImage,
				     newwidth,newheight,(void *)dstImage,
				     element_size,rowsize,myswap_bytes);
	    break;
	case GL_UNSIGNED_INT_2_10_10_10_REV:
	    scaleInternalPackedPixel(4,extract2101010rev,shove2101010rev,
				     width, height,usersImage,
				     newwidth,newheight,(void *)dstImage,
				     element_size,rowsize,myswap_bytes);
	    break;
	default:
	    assert(0);
	    break;
	}
	myswap_bytes = 0;
	rowsize = newwidth * group_size;
	/* Swap dstImage and srcImage */
	__GLU_SWAP_IMAGE(srcImage,dstImage);

	if(levels != 0) { /* use as little memory as possible */
	  {
	     int nextWidth= newwidth/2;
	     int nextHeight= newheight/2;
	     if (nextWidth < 1) nextWidth= 1;
	     if (nextHeight < 1) nextHeight= 1; 

	  memreq = image_size(nextWidth, nextHeight, format, type);
	  }

	  switch(type) {
	  case GL_UNSIGNED_BYTE:
	    dstImage = (GLubyte *)malloc(memreq);
	    break;
	  case GL_BYTE:
	    dstImage = (GLbyte *)malloc(memreq);
	    break;
	  case GL_UNSIGNED_SHORT:
	    dstImage = (GLushort *)malloc(memreq);
	    break;
	  case GL_SHORT:
	    dstImage = (GLshort *)malloc(memreq);
	    break;
	  case GL_UNSIGNED_INT:
	    dstImage = (GLuint *)malloc(memreq);
	    break;
	  case GL_INT:
	    dstImage = (GLint *)malloc(memreq);
	    break;
	  case GL_FLOAT:
	    dstImage = (GLfloat *)malloc(memreq);
	    break;
	  case GL_UNSIGNED_BYTE_3_3_2:
	  case GL_UNSIGNED_BYTE_2_3_3_REV:
	    dstImage = (GLubyte *)malloc(memreq);
	    break;
	  case GL_UNSIGNED_SHORT_5_6_5:
	  case GL_UNSIGNED_SHORT_5_6_5_REV:
	  case GL_UNSIGNED_SHORT_4_4_4_4:
	  case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	  case GL_UNSIGNED_SHORT_5_5_5_1:
	  case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	    dstImage = (GLushort *)malloc(memreq);
	    break;
	  case GL_UNSIGNED_INT_8_8_8_8:
	  case GL_UNSIGNED_INT_8_8_8_8_REV:
	  case GL_UNSIGNED_INT_10_10_10_2:
	  case GL_UNSIGNED_INT_2_10_10_10_REV:
	    dstImage = (GLuint *)malloc(memreq);
	    break;
	  default:
	    return GLU_INVALID_ENUM;
	  }
	  if (dstImage == NULL) {
	    glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
	    glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
	    glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
	    glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
	    glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
	    free(srcImage);
	    return GLU_OUT_OF_MEMORY;
	  }
	}
	/* level userLevel is in srcImage; nothing saved yet */
	level = userLevel;
    }

    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
    if (baseLevel <= level && level <= maxLevel) {
    glTexImage2D(target, level, internalFormat, newwidth, newheight, 0,
		 format, type, (void *)srcImage);
    }

    level++; /* update current level for the loop */
    for (; level <= levels; level++) {
	switch(type) {
	    case GL_UNSIGNED_BYTE:
		halveImage_ubyte(cmpts, newwidth, newheight,
		(GLubyte *)srcImage, (GLubyte *)dstImage, element_size,
		rowsize, group_size);
		break;
	    case GL_BYTE:
		halveImage_byte(cmpts, newwidth, newheight,
		(GLbyte *)srcImage, (GLbyte *)dstImage, element_size,
		rowsize, group_size);
		break;
	    case GL_UNSIGNED_SHORT:
		halveImage_ushort(cmpts, newwidth, newheight,
		(GLushort *)srcImage, (GLushort *)dstImage, element_size,
		rowsize, group_size, myswap_bytes);
		break;
	    case GL_SHORT:
		halveImage_short(cmpts, newwidth, newheight,
		(GLshort *)srcImage, (GLshort *)dstImage, element_size,
		rowsize, group_size, myswap_bytes);
		break;
	    case GL_UNSIGNED_INT:
		halveImage_uint(cmpts, newwidth, newheight,
		(GLuint *)srcImage, (GLuint *)dstImage, element_size,
		rowsize, group_size, myswap_bytes);
		break;
	    case GL_INT:
		halveImage_int(cmpts, newwidth, newheight,
		(GLint *)srcImage, (GLint *)dstImage, element_size,
		rowsize, group_size, myswap_bytes);
		break;
	    case GL_FLOAT:
		halveImage_float(cmpts, newwidth, newheight,
		(GLfloat *)srcImage, (GLfloat *)dstImage, element_size,
		rowsize, group_size, myswap_bytes);
		break;
	    case GL_UNSIGNED_BYTE_3_3_2:
		halveImagePackedPixel(3,extract332,shove332,
				      newwidth,newheight,
				      srcImage,dstImage,element_size,rowsize,
				      myswap_bytes);
		break;
	    case GL_UNSIGNED_BYTE_2_3_3_REV:
		halveImagePackedPixel(3,extract233rev,shove233rev,
				      newwidth,newheight,
				      srcImage,dstImage,element_size,rowsize,
				      myswap_bytes);
		break;
	    case GL_UNSIGNED_SHORT_5_6_5:
		halveImagePackedPixel(3,extract565,shove565,
				      newwidth,newheight,
				      srcImage,dstImage,element_size,rowsize,
				      myswap_bytes);
		break;
	    case GL_UNSIGNED_SHORT_5_6_5_REV:
		halveImagePackedPixel(3,extract565rev,shove565rev,
				      newwidth,newheight,
				      srcImage,dstImage,element_size,rowsize,
				      myswap_bytes);
		break;
	    case GL_UNSIGNED_SHORT_4_4_4_4:
		halveImagePackedPixel(4,extract4444,shove4444,
				      newwidth,newheight,
				      srcImage,dstImage,element_size,rowsize,
				      myswap_bytes);
		break;
	    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
		halveImagePackedPixel(4,extract4444rev,shove4444rev,
				      newwidth,newheight,
				      srcImage,dstImage,element_size,rowsize,
				      myswap_bytes);
		break;
	    case GL_UNSIGNED_SHORT_5_5_5_1:	        
		halveImagePackedPixel(4,extract5551,shove5551,
				      newwidth,newheight,
				      srcImage,dstImage,element_size,rowsize,
				      myswap_bytes);
		break;
	    case GL_UNSIGNED_SHORT_1_5_5_5_REV: 	        
		halveImagePackedPixel(4,extract1555rev,shove1555rev,
				      newwidth,newheight,
				      srcImage,dstImage,element_size,rowsize,
				      myswap_bytes);
		break;
	    case GL_UNSIGNED_INT_8_8_8_8:
		halveImagePackedPixel(4,extract8888,shove8888,
				      newwidth,newheight,
				      srcImage,dstImage,element_size,rowsize,
				      myswap_bytes);
		break;
	    case GL_UNSIGNED_INT_8_8_8_8_REV:
		halveImagePackedPixel(4,extract8888rev,shove8888rev,
				      newwidth,newheight,
				      srcImage,dstImage,element_size,rowsize,
				      myswap_bytes);
		break;
	    case GL_UNSIGNED_INT_10_10_10_2:
		halveImagePackedPixel(4,extract1010102,shove1010102,
				      newwidth,newheight,
				      srcImage,dstImage,element_size,rowsize,
				      myswap_bytes);
		break;
	    case GL_UNSIGNED_INT_2_10_10_10_REV:
		halveImagePackedPixel(4,extract2101010rev,shove2101010rev,
				      newwidth,newheight,
				      srcImage,dstImage,element_size,rowsize,
				      myswap_bytes);
		break;
	    default:
		assert(0);
		break;
	}

	__GLU_SWAP_IMAGE(srcImage,dstImage);

	if (newwidth > 1) { newwidth /= 2; rowsize /= 2;}
	if (newheight > 1) newheight /= 2;
      {
       /* compute amount to pad per row, if any */
       int rowPad= rowsize % psm.unpack_alignment;

       /* should row be padded? */
       if (rowPad == 0) {	/* nope, row should not be padded */
	   /* call tex image with srcImage untouched since it's not padded */
	   if (baseLevel <= level && level <= maxLevel) {
	   glTexImage2D(target, level, internalFormat, newwidth, newheight, 0,
	   format, type, (void *) srcImage);
	   }
       }
       else {			/* yes, row should be padded */
	  /* compute length of new row in bytes, including padding */
	  int newRowLength= rowsize + psm.unpack_alignment - rowPad;
	  int ii; unsigned char *dstTrav, *srcTrav; /* indices for copying */

	  /* allocate new image for mipmap of size newRowLength x newheight */
	  void *newMipmapImage= malloc((size_t) (newRowLength*newheight));
	  if (newMipmapImage == NULL) {
	     /* out of memory so return */
	     glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
	     glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
	     glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
	     glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
	     glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
	     return GLU_OUT_OF_MEMORY;
	  }

	  /* copy image from srcImage into newMipmapImage by rows */
	  for (ii= 0,
	       dstTrav= (unsigned char *) newMipmapImage,
	       srcTrav= (unsigned char *) srcImage;
	       ii< newheight;
	       ii++,
	       dstTrav+= newRowLength, /* make sure the correct distance... */
	       srcTrav+= rowsize) {    /* ...is skipped */
	     memcpy(dstTrav,srcTrav,rowsize);
	     /* note that the pad bytes are not visited and will contain
	      * garbage, which is ok.
	      */
	  }

	  /* ...and use this new image for mipmapping instead */
	  if (baseLevel <= level && level <= maxLevel) {
	  glTexImage2D(target, level, internalFormat, newwidth, newheight, 0,
		       format, type, newMipmapImage);
	  }
	  free(newMipmapImage); /* don't forget to free it! */
       } /* else */
      }
    } /* for level */
    glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);

    free(srcImage); /*if you get to here, a srcImage has always been malloc'ed*/
    if (dstImage) { /* if it's non-rectangular and only 1 level */
      free(dstImage);
    }
    return 0;
} /* gluBuild2DMipmapLevelsCore() */

GLint GLAPIENTRY
gluBuild2DMipmapLevels(GLenum target, GLint internalFormat,
			     GLsizei width, GLsizei height,
			     GLenum format, GLenum type,
			     GLint userLevel, GLint baseLevel, GLint maxLevel,
			     const void *data)
{
   int level, levels;

   int rc= checkMipmapArgs(internalFormat,format,type);
   if (rc != 0) return rc;

   if (width < 1 || height < 1) {
       return GLU_INVALID_VALUE;
   }

   levels = computeLog(width);
   level = computeLog(height);
   if (level > levels) levels=level;

   levels+= userLevel;
   if (!isLegalLevels(userLevel,baseLevel,maxLevel,levels))
      return GLU_INVALID_VALUE;

   return gluBuild2DMipmapLevelsCore(target, internalFormat,
				     width, height,
				     width, height,
				     format, type,
				     userLevel, baseLevel, maxLevel,
				     data);
} /* gluBuild2DMipmapLevels() */

GLint GLAPIENTRY
gluBuild2DMipmaps(GLenum target, GLint internalFormat,
			GLsizei width, GLsizei height,
			GLenum format, GLenum type,
			const void *data)
{
   GLint widthPowerOf2, heightPowerOf2;
   int level, levels;

   int rc= checkMipmapArgs(internalFormat,format,type);
   if (rc != 0) return rc;

   if (width < 1 || height < 1) {
       return GLU_INVALID_VALUE;
   }

   closestFit(target,width,height,internalFormat,format,type,
	      &widthPowerOf2,&heightPowerOf2);

   levels = computeLog(widthPowerOf2);
   level = computeLog(heightPowerOf2);
   if (level > levels) levels=level;

   return gluBuild2DMipmapLevelsCore(target,internalFormat,
				     width, height,
				     widthPowerOf2,heightPowerOf2,
				     format,type,
				     0,0,levels,data);
}  /* gluBuild2DMipmaps() */

#if 0
/*
** This routine is for the limited case in which
**	type == GL_UNSIGNED_BYTE && format != index  &&
**	unpack_alignment = 1 && unpack_swap_bytes == false
**
** so all of the work data can be kept as ubytes instead of shorts.
*/
static int fastBuild2DMipmaps(const PixelStorageModes *psm,
		       GLenum target, GLint components, GLint width,
		     GLint height, GLenum format,
		     GLenum type, void *data)
{
    GLint newwidth, newheight;
    GLint level, levels;
    GLubyte *newImage;
    GLint newImage_width;
    GLint newImage_height;
    GLubyte *otherImage;
    GLubyte *imageTemp;
    GLint memreq;
    GLint cmpts;


#if 0
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);
    newwidth = nearestPower(width);
    if (newwidth > maxsize) newwidth = maxsize;
    newheight = nearestPower(height);
    if (newheight > maxsize) newheight = maxsize;
#else
    closestFit(target,width,height,components,format,type,
	       &newwidth,&newheight);
#endif
    levels = computeLog(newwidth);
    level = computeLog(newheight);
    if (level > levels) levels=level;

    cmpts = elements_per_group(format,type);

    otherImage = NULL;
    /**
    ** No need to copy the user data if its in the packed correctly.
    ** Make sure that later routines don't change that data.
    */
    if (psm->unpack_skip_rows == 0 && psm->unpack_skip_pixels == 0) {
	newImage = (GLubyte *)data;
	newImage_width = width;
	newImage_height = height;
    } else {
	GLint rowsize;
	GLint groups_per_line;
	GLint elements_per_line;
	const GLubyte *start;
	const GLubyte *iter;
	GLubyte *iter2;
	GLint i, j;

	newImage = (GLubyte *)
	    malloc(image_size(width, height, format, GL_UNSIGNED_BYTE));
	newImage_width = width;
	newImage_height = height;
	if (newImage == NULL) {
	    return GLU_OUT_OF_MEMORY;
	}

	/*
	** Abbreviated version of fill_image for this restricted case.
	*/
	if (psm->unpack_row_length > 0) {
	    groups_per_line = psm->unpack_row_length;
	} else {
	    groups_per_line = width;
	}
	rowsize = groups_per_line * cmpts;
	elements_per_line = width * cmpts;
	start = (const GLubyte *) data + psm->unpack_skip_rows * rowsize +
		psm->unpack_skip_pixels * cmpts;
	iter2 = newImage;

	for (i = 0; i < height; i++) {
	    iter = start;
	    for (j = 0; j < elements_per_line; j++) {
		*iter2 = *iter;
		iter++;
		iter2++;
	    }
	    start += rowsize;
	}
    }


    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

    for (level = 0; level <= levels; level++) {
	if (newImage_width == newwidth && newImage_height == newheight) {
	    /* Use newImage for this level */
	    glTexImage2D(target, level, components, newImage_width,
		    newImage_height, 0, format, GL_UNSIGNED_BYTE,
		    (void *) newImage);
	} else {
	    if (otherImage == NULL) {
		memreq =
		    image_size(newwidth, newheight, format, GL_UNSIGNED_BYTE);
		otherImage = (GLubyte *) malloc(memreq);
		if (otherImage == NULL) {
		    glPixelStorei(GL_UNPACK_ALIGNMENT, psm->unpack_alignment);
		    glPixelStorei(GL_UNPACK_SKIP_ROWS, psm->unpack_skip_rows);
		    glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm->unpack_skip_pixels);
		    glPixelStorei(GL_UNPACK_ROW_LENGTH,psm->unpack_row_length);
		    glPixelStorei(GL_UNPACK_SWAP_BYTES,psm->unpack_swap_bytes);
		    return GLU_OUT_OF_MEMORY;
		}
	    }
/*
	    scale_internal_ubyte(cmpts, newImage_width, newImage_height,
		    newImage, newwidth, newheight, otherImage);
*/
	    /* Swap newImage and otherImage */
	    imageTemp = otherImage;
	    otherImage = newImage;
	    newImage = imageTemp;

	    newImage_width = newwidth;
	    newImage_height = newheight;
	    glTexImage2D(target, level, components, newImage_width,
		    newImage_height, 0, format, GL_UNSIGNED_BYTE,
		    (void *) newImage);
	}
	if (newwidth > 1) newwidth /= 2;
	if (newheight > 1) newheight /= 2;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, psm->unpack_alignment);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, psm->unpack_skip_rows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm->unpack_skip_pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, psm->unpack_row_length);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, psm->unpack_swap_bytes);

    if (newImage != (const GLubyte *)data) {
	free((GLbyte *) newImage);
    }
    if (otherImage && otherImage != (const GLubyte *)data) {
	free((GLbyte *) otherImage);
    }
    return 0;
}
#endif

/*
 * Utility Routines
 */
static GLint elements_per_group(GLenum format, GLenum type)
{
    /*
     * Return the number of elements per group of a specified format
     */

    /* If the type is packedpixels then answer is 1 (ignore format) */
    if (type == GL_UNSIGNED_BYTE_3_3_2 ||
	type == GL_UNSIGNED_BYTE_2_3_3_REV ||
	type == GL_UNSIGNED_SHORT_5_6_5 ||
	type == GL_UNSIGNED_SHORT_5_6_5_REV ||
	type == GL_UNSIGNED_SHORT_4_4_4_4 ||
	type == GL_UNSIGNED_SHORT_4_4_4_4_REV ||
	type == GL_UNSIGNED_SHORT_5_5_5_1  ||
	type == GL_UNSIGNED_SHORT_1_5_5_5_REV  ||
	type == GL_UNSIGNED_INT_8_8_8_8 ||
	type == GL_UNSIGNED_INT_8_8_8_8_REV ||
	type == GL_UNSIGNED_INT_10_10_10_2 ||
	type == GL_UNSIGNED_INT_2_10_10_10_REV) {
	return 1;
    }

    /* Types are not packed pixels, so get elements per group */
    switch(format) {
      case GL_RGB:
      case GL_BGR:
	return 3;
      case GL_LUMINANCE_ALPHA:
	return 2;
      case GL_RGBA:
      case GL_BGRA:
	return 4;
      default:
	return 1;
    }
}

static GLfloat bytes_per_element(GLenum type)
{
    /*
     * Return the number of bytes per element, based on the element type
     */
    switch(type) {
      case GL_BITMAP:
	return 1.0 / 8.0;
      case GL_UNSIGNED_SHORT:
	return(sizeof(GLushort));
      case GL_SHORT:
	return(sizeof(GLshort));
      case GL_UNSIGNED_BYTE:
	return(sizeof(GLubyte));
      case GL_BYTE:
	return(sizeof(GLbyte));
      case GL_INT:
	return(sizeof(GLint));
      case GL_UNSIGNED_INT:
	return(sizeof(GLuint));
      case GL_FLOAT:
	return(sizeof(GLfloat));
      case GL_UNSIGNED_BYTE_3_3_2:
      case GL_UNSIGNED_BYTE_2_3_3_REV:  
	return(sizeof(GLubyte));
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_5_6_5_REV:
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	return(sizeof(GLushort));
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_8_8_8_8_REV:
      case GL_UNSIGNED_INT_10_10_10_2:
      case GL_UNSIGNED_INT_2_10_10_10_REV:
	return(sizeof(GLuint));
      default:
	return 4;
    }
}

static GLint is_index(GLenum format)
{
    return format == GL_COLOR_INDEX || format == GL_STENCIL_INDEX;
}

/*
** Compute memory required for internal packed array of data of given type
** and format.
*/
static GLint image_size(GLint width, GLint height, GLenum format, GLenum type)
{
    int bytes_per_row;
    int components;

assert(width > 0);
assert(height > 0);
    components = elements_per_group(format,type);
    if (type == GL_BITMAP) {
	bytes_per_row = (width + 7) / 8;
    } else {
	bytes_per_row = bytes_per_element(type) * width;
    }
    return bytes_per_row * height * components;
}

/*
** Extract array from user's data applying all pixel store modes.
** The internal format used is an array of unsigned shorts.
*/
static void fill_image(const PixelStorageModes *psm,
		       GLint width, GLint height, GLenum format,
		       GLenum type, GLboolean index_format,
		       const void *userdata, GLushort *newimage)
{
    GLint components;
    GLint element_size;
    GLint rowsize;
    GLint padding;
    GLint groups_per_line;
    GLint group_size;
    GLint elements_per_line;
    const GLubyte *start;
    const GLubyte *iter;
    GLushort *iter2;
    GLint i, j, k;
    GLint myswap_bytes;

    myswap_bytes = psm->unpack_swap_bytes;
    components = elements_per_group(format,type);
    if (psm->unpack_row_length > 0) {
	groups_per_line = psm->unpack_row_length;
    } else {
	groups_per_line = width;
    }

    /* All formats except GL_BITMAP fall out trivially */
    if (type == GL_BITMAP) {
	GLint bit_offset;
	GLint current_bit;

	rowsize = (groups_per_line * components + 7) / 8;
	padding = (rowsize % psm->unpack_alignment);
	if (padding) {
	    rowsize += psm->unpack_alignment - padding;
	}
	start = (const GLubyte *) userdata + psm->unpack_skip_rows * rowsize +
		(psm->unpack_skip_pixels * components / 8);
	elements_per_line = width * components;
	iter2 = newimage;
	for (i = 0; i < height; i++) {
	    iter = start;
	    bit_offset = (psm->unpack_skip_pixels * components) % 8;
	    for (j = 0; j < elements_per_line; j++) {
		/* Retrieve bit */
		if (psm->unpack_lsb_first) {
		    current_bit = iter[0] & (1 << bit_offset);
		} else {
		    current_bit = iter[0] & (1 << (7 - bit_offset));
		}
		if (current_bit) {
		    if (index_format) {
			*iter2 = 1;
		    } else {
			*iter2 = 65535;
		    }
		} else {
		    *iter2 = 0;
		}
		bit_offset++;
		if (bit_offset == 8) {
		    bit_offset = 0;
		    iter++;
		}
		iter2++;
	    }
	    start += rowsize;
	}
    } else {
	element_size = bytes_per_element(type);
	group_size = element_size * components;
	if (element_size == 1) myswap_bytes = 0;

	rowsize = groups_per_line * group_size;
	padding = (rowsize % psm->unpack_alignment);
	if (padding) {
	    rowsize += psm->unpack_alignment - padding;
	}
	start = (const GLubyte *) userdata + psm->unpack_skip_rows * rowsize +
		psm->unpack_skip_pixels * group_size;
	elements_per_line = width * components;

	iter2 = newimage;
	for (i = 0; i < height; i++) {
	    iter = start;
	    for (j = 0; j < elements_per_line; j++) {
		Type_Widget widget;
		float extractComponents[4];

		switch(type) {
		  case GL_UNSIGNED_BYTE_3_3_2:
		    extract332(0,iter,extractComponents);
		    for (k = 0; k < 3; k++) {
		      *iter2++ = (GLushort)(extractComponents[k]*65535);
		    }
		    break;
		  case GL_UNSIGNED_BYTE_2_3_3_REV:
		    extract233rev(0,iter,extractComponents);
		    for (k = 0; k < 3; k++) {
		      *iter2++ = (GLushort)(extractComponents[k]*65535);
		    }
		    break;
		  case GL_UNSIGNED_BYTE:
		    if (index_format) {
			*iter2++ = *iter;
		    } else {
			*iter2++ = (*iter) * 257;
		    }
		    break;
		  case GL_BYTE:
		    if (index_format) {
			*iter2++ = *((const GLbyte *) iter);
		    } else {
			/* rough approx */
			*iter2++ = (*((const GLbyte *) iter)) * 516;
		    }
		    break;
		  case GL_UNSIGNED_SHORT_5_6_5: 		        
		    extract565(myswap_bytes,iter,extractComponents);
		    for (k = 0; k < 3; k++) {
		      *iter2++ = (GLushort)(extractComponents[k]*65535);
		    }
		    break;
		  case GL_UNSIGNED_SHORT_5_6_5_REV:		        
		    extract565rev(myswap_bytes,iter,extractComponents);
		    for (k = 0; k < 3; k++) {
		      *iter2++ = (GLushort)(extractComponents[k]*65535);
		    }
		    break;
		  case GL_UNSIGNED_SHORT_4_4_4_4:	        
		    extract4444(myswap_bytes,iter,extractComponents);
		    for (k = 0; k < 4; k++) {
		      *iter2++ = (GLushort)(extractComponents[k]*65535);
		    }
		    break;
		  case GL_UNSIGNED_SHORT_4_4_4_4_REV:	        
		    extract4444rev(myswap_bytes,iter,extractComponents);
		    for (k = 0; k < 4; k++) {
		      *iter2++ = (GLushort)(extractComponents[k]*65535);
		    }
		    break;
		  case GL_UNSIGNED_SHORT_5_5_5_1:	        
		    extract5551(myswap_bytes,iter,extractComponents);
		    for (k = 0; k < 4; k++) {
		      *iter2++ = (GLushort)(extractComponents[k]*65535);
		    }
		    break;
		  case GL_UNSIGNED_SHORT_1_5_5_5_REV:
		    extract1555rev(myswap_bytes,iter,extractComponents);
		    for (k = 0; k < 4; k++) {
		      *iter2++ = (GLushort)(extractComponents[k]*65535);
		    }
		    break;
		  case GL_UNSIGNED_SHORT:
		  case GL_SHORT:
		    if (myswap_bytes) {
			widget.ub[0] = iter[1];
			widget.ub[1] = iter[0];
		    } else {
			widget.ub[0] = iter[0];
			widget.ub[1] = iter[1];
		    }
		    if (type == GL_SHORT) {
			if (index_format) {
			    *iter2++ = widget.s[0];
			} else {
			    /* rough approx */
			    *iter2++ = widget.s[0]*2;
			}
		    } else {
			*iter2++ = widget.us[0];
		    }
		    break;
		  case GL_UNSIGNED_INT_8_8_8_8:         
		    extract8888(myswap_bytes,iter,extractComponents);
		    for (k = 0; k < 4; k++) {
		      *iter2++ = (GLushort)(extractComponents[k]*65535);
		    }
		    break;
		  case GL_UNSIGNED_INT_8_8_8_8_REV:	        
		    extract8888rev(myswap_bytes,iter,extractComponents);
		    for (k = 0; k < 4; k++) {
		      *iter2++ = (GLushort)(extractComponents[k]*65535);
		    }
		    break;
		  case GL_UNSIGNED_INT_10_10_10_2:	        
		    extract1010102(myswap_bytes,iter,extractComponents);
		    for (k = 0; k < 4; k++) {
		      *iter2++ = (GLushort)(extractComponents[k]*65535);
		    }
		    break;
		  case GL_UNSIGNED_INT_2_10_10_10_REV:
		    extract2101010rev(myswap_bytes,iter,extractComponents);
		    for (k = 0; k < 4; k++) {
		      *iter2++ = (GLushort)(extractComponents[k]*65535);
		    }
		    break;
		  case GL_INT:
		  case GL_UNSIGNED_INT:
		  case GL_FLOAT:
		    if (myswap_bytes) {
			widget.ub[0] = iter[3];
			widget.ub[1] = iter[2];
			widget.ub[2] = iter[1];
			widget.ub[3] = iter[0];
		    } else {
			widget.ub[0] = iter[0];
			widget.ub[1] = iter[1];
			widget.ub[2] = iter[2];
			widget.ub[3] = iter[3];
		    }
		    if (type == GL_FLOAT) {
			if (index_format) {
			    *iter2++ = widget.f;
			} else {
			    *iter2++ = 65535 * widget.f;
			}
		    } else if (type == GL_UNSIGNED_INT) {
			if (index_format) {
			    *iter2++ = widget.ui;
			} else {
			    *iter2++ = widget.ui >> 16;
			}
		    } else {
			if (index_format) {
			    *iter2++ = widget.i;
			} else {
			    *iter2++ = widget.i >> 15;
			}
		    }
		    break;
		}
		iter += element_size;
	    } /* for j */
	    start += rowsize;
#if 1
	    /* want 'iter' pointing at start, not within, row for assertion
	     * purposes
	     */
	    iter= start;        
#endif
	} /* for i */

       /* iterators should be one byte past end */
       if (!isTypePackedPixel(type)) {
	  assert(iter2 == &newimage[width*height*components]);
       }
       else {
	  assert(iter2 == &newimage[width*height*
				    elements_per_group(format,0)]);
       }
       assert( iter == &((const GLubyte *)userdata)[rowsize*height +
					psm->unpack_skip_rows * rowsize +
					psm->unpack_skip_pixels * group_size] );

    } /* else */
} /* fill_image() */

/*
** Insert array into user's data applying all pixel store modes.
** The internal format is an array of unsigned shorts.
** empty_image() because it is the opposite of fill_image().
*/
static void empty_image(const PixelStorageModes *psm,
			GLint width, GLint height, GLenum format,
			GLenum type, GLboolean index_format,
			const GLushort *oldimage, void *userdata)
{
    GLint components;
    GLint element_size;
    GLint rowsize;
    GLint padding;
    GLint groups_per_line;
    GLint group_size;
    GLint elements_per_line;
    GLubyte *start;
    GLubyte *iter;
    const GLushort *iter2;
    GLint i, j, k;
    GLint myswap_bytes;

    myswap_bytes = psm->pack_swap_bytes;
    components = elements_per_group(format,type);
    if (psm->pack_row_length > 0) {
	groups_per_line = psm->pack_row_length;
    } else {
	groups_per_line = width;
    }

    /* All formats except GL_BITMAP fall out trivially */
    if (type == GL_BITMAP) {
	GLint bit_offset;
	GLint current_bit;

	rowsize = (groups_per_line * components + 7) / 8;
	padding = (rowsize % psm->pack_alignment);
	if (padding) {
	    rowsize += psm->pack_alignment - padding;
	}
	start = (GLubyte *) userdata + psm->pack_skip_rows * rowsize +
		(psm->pack_skip_pixels * components / 8);
	elements_per_line = width * components;
	iter2 = oldimage;
	for (i = 0; i < height; i++) {
	    iter = start;
	    bit_offset = (psm->pack_skip_pixels * components) % 8;
	    for (j = 0; j < elements_per_line; j++) {
		if (index_format) {
		    current_bit = iter2[0] & 1;
		} else {
		    if (iter2[0] > 32767) {
			current_bit = 1;
		    } else {
			current_bit = 0;
		    }
		}

		if (current_bit) {
		    if (psm->pack_lsb_first) {
			*iter |= (1 << bit_offset);
		    } else {
			*iter |= (1 << (7 - bit_offset));
		    }
		} else {
		    if (psm->pack_lsb_first) {
			*iter &= ~(1 << bit_offset);
		    } else {
			*iter &= ~(1 << (7 - bit_offset));
		    }
		}

		bit_offset++;
		if (bit_offset == 8) {
		    bit_offset = 0;
		    iter++;
		}
		iter2++;
	    }
	    start += rowsize;
	}
    } else {
	float shoveComponents[4];

	element_size = bytes_per_element(type);
	group_size = element_size * components;
	if (element_size == 1) myswap_bytes = 0;

	rowsize = groups_per_line * group_size;
	padding = (rowsize % psm->pack_alignment);
	if (padding) {
	    rowsize += psm->pack_alignment - padding;
	}
	start = (GLubyte *) userdata + psm->pack_skip_rows * rowsize +
		psm->pack_skip_pixels * group_size;
	elements_per_line = width * components;

	iter2 = oldimage;
	for (i = 0; i < height; i++) {
	    iter = start;
	    for (j = 0; j < elements_per_line; j++) {
		Type_Widget widget;

		switch(type) {
		  case GL_UNSIGNED_BYTE_3_3_2:
		    for (k = 0; k < 3; k++) {
		       shoveComponents[k]= *iter2++ / 65535.0;
		    }
		    shove332(shoveComponents,0,(void *)iter);
		    break;
		  case GL_UNSIGNED_BYTE_2_3_3_REV:
		    for (k = 0; k < 3; k++) {
		       shoveComponents[k]= *iter2++ / 65535.0;
		    }
		    shove233rev(shoveComponents,0,(void *)iter);
		    break;
		  case GL_UNSIGNED_BYTE:
		    if (index_format) {
			*iter = *iter2++;
		    } else {
			*iter = *iter2++ >> 8;
		    }
		    break;
		  case GL_BYTE:
		    if (index_format) {
			*((GLbyte *) iter) = *iter2++;
		    } else {
			*((GLbyte *) iter) = *iter2++ >> 9;
		    }
		    break;
		  case GL_UNSIGNED_SHORT_5_6_5:         
		    for (k = 0; k < 3; k++) {
		       shoveComponents[k]= *iter2++ / 65535.0;
		    }
		    shove565(shoveComponents,0,(void *)&widget.us[0]);
		    if (myswap_bytes) {
		       iter[0] = widget.ub[1];
		       iter[1] = widget.ub[0];
		    }
		    else {
		       *(GLushort *)iter = widget.us[0];
		    }
		    break;
		  case GL_UNSIGNED_SHORT_5_6_5_REV:	        
		    for (k = 0; k < 3; k++) {
		       shoveComponents[k]= *iter2++ / 65535.0;
		    }
		    shove565rev(shoveComponents,0,(void *)&widget.us[0]);
		    if (myswap_bytes) {
		       iter[0] = widget.ub[1];
		       iter[1] = widget.ub[0];
		    }
		    else {
		       *(GLushort *)iter = widget.us[0];
		    }
		    break;
		  case GL_UNSIGNED_SHORT_4_4_4_4:
		    for (k = 0; k < 4; k++) {
		       shoveComponents[k]= *iter2++ / 65535.0;
		    }
		    shove4444(shoveComponents,0,(void *)&widget.us[0]);
		    if (myswap_bytes) {
		       iter[0] = widget.ub[1];
		       iter[1] = widget.ub[0];
		    } else {
		       *(GLushort *)iter = widget.us[0];
		    }
		    break;
		  case GL_UNSIGNED_SHORT_4_4_4_4_REV:
		    for (k = 0; k < 4; k++) {
		       shoveComponents[k]= *iter2++ / 65535.0;
		    }
		    shove4444rev(shoveComponents,0,(void *)&widget.us[0]);
		    if (myswap_bytes) {
		       iter[0] = widget.ub[1];
		       iter[1] = widget.ub[0];
		    } else {
		       *(GLushort *)iter = widget.us[0];
		    }
		    break;
		  case GL_UNSIGNED_SHORT_5_5_5_1:
		    for (k = 0; k < 4; k++) {
		       shoveComponents[k]= *iter2++ / 65535.0;
		    }
		    shove5551(shoveComponents,0,(void *)&widget.us[0]);
		    if (myswap_bytes) {
		       iter[0] = widget.ub[1];
		       iter[1] = widget.ub[0];
		    } else {
		       *(GLushort *)iter = widget.us[0];
		    }
		    break;
		  case GL_UNSIGNED_SHORT_1_5_5_5_REV:
		    for (k = 0; k < 4; k++) {
		       shoveComponents[k]= *iter2++ / 65535.0;
		    }
		    shove1555rev(shoveComponents,0,(void *)&widget.us[0]);
		    if (myswap_bytes) {
		       iter[0] = widget.ub[1];
		       iter[1] = widget.ub[0];
		    } else {
		       *(GLushort *)iter = widget.us[0];
		    }
		    break;
		  case GL_UNSIGNED_SHORT:
		  case GL_SHORT:
		    if (type == GL_SHORT) {
			if (index_format) {
			    widget.s[0] = *iter2++;
			} else {
			    widget.s[0] = *iter2++ >> 1;
			}
		    } else {
			widget.us[0] = *iter2++;
		    }
		    if (myswap_bytes) {
			iter[0] = widget.ub[1];
			iter[1] = widget.ub[0];
		    } else {
			iter[0] = widget.ub[0];
			iter[1] = widget.ub[1];
		    }
		    break;
		  case GL_UNSIGNED_INT_8_8_8_8:
		    for (k = 0; k < 4; k++) {
		       shoveComponents[k]= *iter2++ / 65535.0;
		    }
		    shove8888(shoveComponents,0,(void *)&widget.ui);
		    if (myswap_bytes) {
			iter[3] = widget.ub[0];
			iter[2] = widget.ub[1];
			iter[1] = widget.ub[2];
			iter[0] = widget.ub[3];
		    } else {
			*(GLuint *)iter= widget.ui;
		    }

		    break;
		  case GL_UNSIGNED_INT_8_8_8_8_REV:
		    for (k = 0; k < 4; k++) {
		       shoveComponents[k]= *iter2++ / 65535.0;
		    }
		    shove8888rev(shoveComponents,0,(void *)&widget.ui);
		    if (myswap_bytes) {
			iter[3] = widget.ub[0];
			iter[2] = widget.ub[1];
			iter[1] = widget.ub[2];
			iter[0] = widget.ub[3];
		    } else {
			*(GLuint *)iter= widget.ui;
		    }
		    break;
		  case GL_UNSIGNED_INT_10_10_10_2:
		    for (k = 0; k < 4; k++) {
		       shoveComponents[k]= *iter2++ / 65535.0;
		    }
		    shove1010102(shoveComponents,0,(void *)&widget.ui);
		    if (myswap_bytes) {
			iter[3] = widget.ub[0];
			iter[2] = widget.ub[1];
			iter[1] = widget.ub[2];
			iter[0] = widget.ub[3];
		    } else {
			*(GLuint *)iter= widget.ui;
		    }
		    break;
		  case GL_UNSIGNED_INT_2_10_10_10_REV:
		    for (k = 0; k < 4; k++) {
		       shoveComponents[k]= *iter2++ / 65535.0;
		    }
		    shove2101010rev(shoveComponents,0,(void *)&widget.ui);
		    if (myswap_bytes) {
			iter[3] = widget.ub[0];
			iter[2] = widget.ub[1];
			iter[1] = widget.ub[2];
			iter[0] = widget.ub[3];
		    } else {
			*(GLuint *)iter= widget.ui;
		    }
		    break;
		  case GL_INT:
		  case GL_UNSIGNED_INT:
		  case GL_FLOAT:
		    if (type == GL_FLOAT) {
			if (index_format) {
			    widget.f = *iter2++;
			} else {
			    widget.f = *iter2++ / (float) 65535.0;
			}
		    } else if (type == GL_UNSIGNED_INT) {
			if (index_format) {
			    widget.ui = *iter2++;
			} else {
			    widget.ui = (unsigned int) *iter2++ * 65537;
			}
		    } else {
			if (index_format) {
			    widget.i = *iter2++;
			} else {
			    widget.i = ((unsigned int) *iter2++ * 65537)/2;
			}
		    }
		    if (myswap_bytes) {
			iter[3] = widget.ub[0];
			iter[2] = widget.ub[1];
			iter[1] = widget.ub[2];
			iter[0] = widget.ub[3];
		    } else {
			iter[0] = widget.ub[0];
			iter[1] = widget.ub[1];
			iter[2] = widget.ub[2];
			iter[3] = widget.ub[3];
		    }
		    break;
		}
		iter += element_size;
	    } /* for j */
	    start += rowsize;
#if 1
	    /* want 'iter' pointing at start, not within, row for assertion
	     * purposes
	     */
	    iter= start;        
#endif
	} /* for i */

	/* iterators should be one byte past end */
	if (!isTypePackedPixel(type)) {
	   assert(iter2 == &oldimage[width*height*components]);
	}
	else {
	   assert(iter2 == &oldimage[width*height*
				     elements_per_group(format,0)]);
	}
	assert( iter == &((GLubyte *)userdata)[rowsize*height +
					psm->pack_skip_rows * rowsize +
					psm->pack_skip_pixels * group_size] );

    } /* else */
} /* empty_image() */

/*--------------------------------------------------------------------------
 * Decimation of packed pixel types
 *--------------------------------------------------------------------------
 */
static void extract332(int isSwap,
		       const void *packedPixel, GLfloat extractComponents[])
{
   GLubyte ubyte= *(const GLubyte *)packedPixel;

   isSwap= isSwap;		/* turn off warnings */

   /* 11100000 == 0xe0 */
   /* 00011100 == 0x1c */
   /* 00000011 == 0x03 */

   extractComponents[0]=   (float)((ubyte & 0xe0)  >> 5) / 7.0;
   extractComponents[1]=   (float)((ubyte & 0x1c)  >> 2) / 7.0; /* 7 = 2^3-1 */
   extractComponents[2]=   (float)((ubyte & 0x03)      ) / 3.0; /* 3 = 2^2-1 */
} /* extract332() */

static void shove332(const GLfloat shoveComponents[],
		     int index, void *packedPixel)      
{
   /* 11100000 == 0xe0 */
   /* 00011100 == 0x1c */
   /* 00000011 == 0x03 */

   assert(0.0 <= shoveComponents[0] && shoveComponents[0] <= 1.0);
   assert(0.0 <= shoveComponents[1] && shoveComponents[1] <= 1.0);
   assert(0.0 <= shoveComponents[2] && shoveComponents[2] <= 1.0);

   /* due to limited precision, need to round before shoving */
   ((GLubyte *)packedPixel)[index]  =
     ((GLubyte)((shoveComponents[0] * 7)+0.5)  << 5) & 0xe0;
   ((GLubyte *)packedPixel)[index] |=
     ((GLubyte)((shoveComponents[1] * 7)+0.5)  << 2) & 0x1c;
   ((GLubyte *)packedPixel)[index]  |=
     ((GLubyte)((shoveComponents[2] * 3)+0.5)	   ) & 0x03;
} /* shove332() */

static void extract233rev(int isSwap,
			  const void *packedPixel, GLfloat extractComponents[])
{
   GLubyte ubyte= *(const GLubyte *)packedPixel;

   isSwap= isSwap;		/* turn off warnings */

   /* 0000,0111 == 0x07 */
   /* 0011,1000 == 0x38 */
   /* 1100,0000 == 0xC0 */

   extractComponents[0]= (float)((ubyte & 0x07)     ) / 7.0;
   extractComponents[1]= (float)((ubyte & 0x38) >> 3) / 7.0;
   extractComponents[2]= (float)((ubyte & 0xC0) >> 6) / 3.0;
} /* extract233rev() */

static void shove233rev(const GLfloat shoveComponents[],
			int index, void *packedPixel)   
{
   /* 0000,0111 == 0x07 */
   /* 0011,1000 == 0x38 */
   /* 1100,0000 == 0xC0 */

   assert(0.0 <= shoveComponents[0] && shoveComponents[0] <= 1.0);
   assert(0.0 <= shoveComponents[1] && shoveComponents[1] <= 1.0);
   assert(0.0 <= shoveComponents[2] && shoveComponents[2] <= 1.0);

   /* due to limited precision, need to round before shoving */
   ((GLubyte *)packedPixel)[index] =
     ((GLubyte)((shoveComponents[0] * 7.0)+0.5)     ) & 0x07;
   ((GLubyte *)packedPixel)[index]|=
     ((GLubyte)((shoveComponents[1] * 7.0)+0.5) << 3) & 0x38;
   ((GLubyte *)packedPixel)[index]|=
     ((GLubyte)((shoveComponents[2] * 3.0)+0.5) << 6) & 0xC0;
} /* shove233rev() */

static void extract565(int isSwap,
		       const void *packedPixel, GLfloat extractComponents[])
{
   GLushort ushort;

   if (isSwap) {
     ushort= __GLU_SWAP_2_BYTES(packedPixel);
   }
   else {
     ushort= *(const GLushort *)packedPixel;
   }

   /* 11111000,00000000 == 0xf800 */
   /* 00000111,11100000 == 0x07e0 */
   /* 00000000,00011111 == 0x001f */

   extractComponents[0]=(float)((ushort & 0xf800) >> 11) / 31.0;/* 31 = 2^5-1*/
   extractComponents[1]=(float)((ushort & 0x07e0) >>  5) / 63.0;/* 63 = 2^6-1*/
   extractComponents[2]=(float)((ushort & 0x001f)      ) / 31.0;
} /* extract565() */

static void shove565(const GLfloat shoveComponents[],
		     int index,void *packedPixel)
{
   /* 11111000,00000000 == 0xf800 */
   /* 00000111,11100000 == 0x07e0 */
   /* 00000000,00011111 == 0x001f */

   assert(0.0 <= shoveComponents[0] && shoveComponents[0] <= 1.0);
   assert(0.0 <= shoveComponents[1] && shoveComponents[1] <= 1.0);
   assert(0.0 <= shoveComponents[2] && shoveComponents[2] <= 1.0);

   /* due to limited precision, need to round before shoving */
   ((GLushort *)packedPixel)[index] =
     ((GLushort)((shoveComponents[0] * 31)+0.5) << 11) & 0xf800;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[1] * 63)+0.5) <<  5) & 0x07e0;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[2] * 31)+0.5)      ) & 0x001f;
} /* shove565() */

static void extract565rev(int isSwap,
			  const void *packedPixel, GLfloat extractComponents[])
{
   GLushort ushort;

   if (isSwap) {
     ushort= __GLU_SWAP_2_BYTES(packedPixel);
   }
   else {
     ushort= *(const GLushort *)packedPixel;
   }

   /* 00000000,00011111 == 0x001f */
   /* 00000111,11100000 == 0x07e0 */
   /* 11111000,00000000 == 0xf800 */

   extractComponents[0]= (float)((ushort & 0x001F)	) / 31.0;
   extractComponents[1]= (float)((ushort & 0x07E0) >>  5) / 63.0;
   extractComponents[2]= (float)((ushort & 0xF800) >> 11) / 31.0;
} /* extract565rev() */

static void shove565rev(const GLfloat shoveComponents[],
			int index,void *packedPixel)
{
   /* 00000000,00011111 == 0x001f */
   /* 00000111,11100000 == 0x07e0 */
   /* 11111000,00000000 == 0xf800 */

   assert(0.0 <= shoveComponents[0] && shoveComponents[0] <= 1.0);
   assert(0.0 <= shoveComponents[1] && shoveComponents[1] <= 1.0);
   assert(0.0 <= shoveComponents[2] && shoveComponents[2] <= 1.0);

   /* due to limited precision, need to round before shoving */
   ((GLushort *)packedPixel)[index] =
     ((GLushort)((shoveComponents[0] * 31.0)+0.5)      ) & 0x001F;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[1] * 63.0)+0.5) <<  5) & 0x07E0;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[2] * 31.0)+0.5) << 11) & 0xF800;
} /* shove565rev() */

static void extract4444(int isSwap,const void *packedPixel,
			GLfloat extractComponents[])
{
   GLushort ushort;

   if (isSwap) {
     ushort= __GLU_SWAP_2_BYTES(packedPixel);
   }
   else {
     ushort= *(const GLushort *)packedPixel;
   }

   /* 11110000,00000000 == 0xf000 */
   /* 00001111,00000000 == 0x0f00 */
   /* 00000000,11110000 == 0x00f0 */
   /* 00000000,00001111 == 0x000f */

   extractComponents[0]= (float)((ushort & 0xf000) >> 12) / 15.0;/* 15=2^4-1 */
   extractComponents[1]= (float)((ushort & 0x0f00) >>  8) / 15.0;
   extractComponents[2]= (float)((ushort & 0x00f0) >>  4) / 15.0;
   extractComponents[3]= (float)((ushort & 0x000f)	) / 15.0;
} /* extract4444() */

static void shove4444(const GLfloat shoveComponents[],
		      int index,void *packedPixel)
{
   assert(0.0 <= shoveComponents[0] && shoveComponents[0] <= 1.0);
   assert(0.0 <= shoveComponents[1] && shoveComponents[1] <= 1.0);
   assert(0.0 <= shoveComponents[2] && shoveComponents[2] <= 1.0);
   assert(0.0 <= shoveComponents[3] && shoveComponents[3] <= 1.0);

   /* due to limited precision, need to round before shoving */
   ((GLushort *)packedPixel)[index] =
     ((GLushort)((shoveComponents[0] * 15)+0.5) << 12) & 0xf000;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[1] * 15)+0.5) <<  8) & 0x0f00;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[2] * 15)+0.5) <<  4) & 0x00f0;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[3] * 15)+0.5)      ) & 0x000f;
} /* shove4444() */

static void extract4444rev(int isSwap,const void *packedPixel,
			   GLfloat extractComponents[])
{
   GLushort ushort;

   if (isSwap) {
     ushort= __GLU_SWAP_2_BYTES(packedPixel);
   }
   else {
     ushort= *(const GLushort *)packedPixel;
   }

   /* 00000000,00001111 == 0x000f */
   /* 00000000,11110000 == 0x00f0 */
   /* 00001111,00000000 == 0x0f00 */
   /* 11110000,00000000 == 0xf000 */

   /* 15 = 2^4-1 */
   extractComponents[0]= (float)((ushort & 0x000F)	) / 15.0;
   extractComponents[1]= (float)((ushort & 0x00F0) >>  4) / 15.0;
   extractComponents[2]= (float)((ushort & 0x0F00) >>  8) / 15.0;
   extractComponents[3]= (float)((ushort & 0xF000) >> 12) / 15.0;
} /* extract4444rev() */

static void shove4444rev(const GLfloat shoveComponents[],
			 int index,void *packedPixel)
{
   /* 00000000,00001111 == 0x000f */
   /* 00000000,11110000 == 0x00f0 */
   /* 00001111,00000000 == 0x0f00 */
   /* 11110000,00000000 == 0xf000 */

   assert(0.0 <= shoveComponents[0] && shoveComponents[0] <= 1.0);
   assert(0.0 <= shoveComponents[1] && shoveComponents[1] <= 1.0);
   assert(0.0 <= shoveComponents[2] && shoveComponents[2] <= 1.0);
   assert(0.0 <= shoveComponents[3] && shoveComponents[3] <= 1.0);

   /* due to limited precision, need to round before shoving */
   ((GLushort *)packedPixel)[index] =
     ((GLushort)((shoveComponents[0] * 15)+0.5)      ) & 0x000F;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[1] * 15)+0.5) <<  4) & 0x00F0;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[2] * 15)+0.5) <<  8) & 0x0F00;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[3] * 15)+0.5) << 12) & 0xF000;
} /* shove4444rev() */

static void extract5551(int isSwap,const void *packedPixel,
			GLfloat extractComponents[])
{
   GLushort ushort;

   if (isSwap) {
     ushort= __GLU_SWAP_2_BYTES(packedPixel);
   }
   else {
     ushort= *(const GLushort *)packedPixel;
   }

   /* 11111000,00000000 == 0xf800 */
   /* 00000111,11000000 == 0x07c0 */
   /* 00000000,00111110 == 0x003e */
   /* 00000000,00000001 == 0x0001 */

   extractComponents[0]=(float)((ushort & 0xf800) >> 11) / 31.0;/* 31 = 2^5-1*/
   extractComponents[1]=(float)((ushort & 0x07c0) >>  6) / 31.0;
   extractComponents[2]=(float)((ushort & 0x003e) >>  1) / 31.0;
   extractComponents[3]=(float)((ushort & 0x0001)      );
} /* extract5551() */

static void shove5551(const GLfloat shoveComponents[],
		      int index,void *packedPixel)
{
   /* 11111000,00000000 == 0xf800 */
   /* 00000111,11000000 == 0x07c0 */
   /* 00000000,00111110 == 0x003e */
   /* 00000000,00000001 == 0x0001 */

   assert(0.0 <= shoveComponents[0] && shoveComponents[0] <= 1.0);
   assert(0.0 <= shoveComponents[1] && shoveComponents[1] <= 1.0);
   assert(0.0 <= shoveComponents[2] && shoveComponents[2] <= 1.0);
   assert(0.0 <= shoveComponents[3] && shoveComponents[3] <= 1.0);

   /* due to limited precision, need to round before shoving */
   ((GLushort *)packedPixel)[index]  =
     ((GLushort)((shoveComponents[0] * 31)+0.5) << 11) & 0xf800;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[1] * 31)+0.5) <<  6) & 0x07c0;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[2] * 31)+0.5) <<  1) & 0x003e;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[3])+0.5)	     ) & 0x0001;
} /* shove5551() */

static void extract1555rev(int isSwap,const void *packedPixel,
			   GLfloat extractComponents[])
{
   GLushort ushort;

   if (isSwap) {
     ushort= __GLU_SWAP_2_BYTES(packedPixel);
   }
   else {
     ushort= *(const GLushort *)packedPixel;
   }

   /* 00000000,00011111 == 0x001F */
   /* 00000011,11100000 == 0x03E0 */
   /* 01111100,00000000 == 0x7C00 */
   /* 10000000,00000000 == 0x8000 */

   /* 31 = 2^5-1 */
   extractComponents[0]= (float)((ushort & 0x001F)	) / 31.0;
   extractComponents[1]= (float)((ushort & 0x03E0) >>  5) / 31.0;
   extractComponents[2]= (float)((ushort & 0x7C00) >> 10) / 31.0;
   extractComponents[3]= (float)((ushort & 0x8000) >> 15);
} /* extract1555rev() */

static void shove1555rev(const GLfloat shoveComponents[],
			 int index,void *packedPixel)
{
   /* 00000000,00011111 == 0x001F */
   /* 00000011,11100000 == 0x03E0 */
   /* 01111100,00000000 == 0x7C00 */
   /* 10000000,00000000 == 0x8000 */

   assert(0.0 <= shoveComponents[0] && shoveComponents[0] <= 1.0);
   assert(0.0 <= shoveComponents[1] && shoveComponents[1] <= 1.0);
   assert(0.0 <= shoveComponents[2] && shoveComponents[2] <= 1.0);
   assert(0.0 <= shoveComponents[3] && shoveComponents[3] <= 1.0);

   /* due to limited precision, need to round before shoving */
   ((GLushort *)packedPixel)[index] =
     ((GLushort)((shoveComponents[0] * 31)+0.5)      ) & 0x001F;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[1] * 31)+0.5) <<  5) & 0x03E0;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[2] * 31)+0.5) << 10) & 0x7C00;
   ((GLushort *)packedPixel)[index]|=
     ((GLushort)((shoveComponents[3])+0.5)	<< 15) & 0x8000;
} /* shove1555rev() */

static void extract8888(int isSwap,
			const void *packedPixel, GLfloat extractComponents[])
{
   GLuint uint;

   if (isSwap) {
     uint= __GLU_SWAP_4_BYTES(packedPixel);
   }
   else {
     uint= *(const GLuint *)packedPixel;
   }

   /* 11111111,00000000,00000000,00000000 == 0xff000000 */
   /* 00000000,11111111,00000000,00000000 == 0x00ff0000 */
   /* 00000000,00000000,11111111,00000000 == 0x0000ff00 */
   /* 00000000,00000000,00000000,11111111 == 0x000000ff */

   /* 255 = 2^8-1 */
   extractComponents[0]= (float)((uint & 0xff000000) >> 24) / 255.0;
   extractComponents[1]= (float)((uint & 0x00ff0000) >> 16) / 255.0;
   extractComponents[2]= (float)((uint & 0x0000ff00) >>  8) / 255.0;
   extractComponents[3]= (float)((uint & 0x000000ff)	  ) / 255.0;
} /* extract8888() */

static void shove8888(const GLfloat shoveComponents[],
		      int index,void *packedPixel)
{
   /* 11111111,00000000,00000000,00000000 == 0xff000000 */
   /* 00000000,11111111,00000000,00000000 == 0x00ff0000 */
   /* 00000000,00000000,11111111,00000000 == 0x0000ff00 */
   /* 00000000,00000000,00000000,11111111 == 0x000000ff */

   assert(0.0 <= shoveComponents[0] && shoveComponents[0] <= 1.0);
   assert(0.0 <= shoveComponents[1] && shoveComponents[1] <= 1.0);
   assert(0.0 <= shoveComponents[2] && shoveComponents[2] <= 1.0);
   assert(0.0 <= shoveComponents[3] && shoveComponents[3] <= 1.0);

   /* due to limited precision, need to round before shoving */
   ((GLuint *)packedPixel)[index] =
     ((GLuint)((shoveComponents[0] * 255)+0.5) << 24) & 0xff000000;
   ((GLuint *)packedPixel)[index]|=
     ((GLuint)((shoveComponents[1] * 255)+0.5) << 16) & 0x00ff0000;
   ((GLuint *)packedPixel)[index]|=
     ((GLuint)((shoveComponents[2] * 255)+0.5) <<  8) & 0x0000ff00;
   ((GLuint *)packedPixel)[index]|=
     ((GLuint)((shoveComponents[3] * 255)+0.5)	    ) & 0x000000ff;
} /* shove8888() */

static void extract8888rev(int isSwap,
			   const void *packedPixel,GLfloat extractComponents[])
{
   GLuint uint;

   if (isSwap) {
     uint= __GLU_SWAP_4_BYTES(packedPixel);
   }
   else {
     uint= *(const GLuint *)packedPixel;
   }

   /* 00000000,00000000,00000000,11111111 == 0x000000ff */
   /* 00000000,00000000,11111111,00000000 == 0x0000ff00 */
   /* 00000000,11111111,00000000,00000000 == 0x00ff0000 */
   /* 11111111,00000000,00000000,00000000 == 0xff000000 */

   /* 255 = 2^8-1 */
   extractComponents[0]= (float)((uint & 0x000000FF)	  ) / 255.0;
   extractComponents[1]= (float)((uint & 0x0000FF00) >>  8) / 255.0;
   extractComponents[2]= (float)((uint & 0x00FF0000) >> 16) / 255.0;
   extractComponents[3]= (float)((uint & 0xFF000000) >> 24) / 255.0;
} /* extract8888rev() */

static void shove8888rev(const GLfloat shoveComponents[],
			 int index,void *packedPixel)
{
   /* 00000000,00000000,00000000,11111111 == 0x000000ff */
   /* 00000000,00000000,11111111,00000000 == 0x0000ff00 */
   /* 00000000,11111111,00000000,00000000 == 0x00ff0000 */
   /* 11111111,00000000,00000000,00000000 == 0xff000000 */

   assert(0.0 <= shoveComponents[0] && shoveComponents[0] <= 1.0);
   assert(0.0 <= shoveComponents[1] && shoveComponents[1] <= 1.0);
   assert(0.0 <= shoveComponents[2] && shoveComponents[2] <= 1.0);
   assert(0.0 <= shoveComponents[3] && shoveComponents[3] <= 1.0);

   /* due to limited precision, need to round before shoving */
   ((GLuint *)packedPixel)[index] =
     ((GLuint)((shoveComponents[0] * 255)+0.5)	    ) & 0x000000FF;
   ((GLuint *)packedPixel)[index]|=
     ((GLuint)((shoveComponents[1] * 255)+0.5) <<  8) & 0x0000FF00;
   ((GLuint *)packedPixel)[index]|=
     ((GLuint)((shoveComponents[2] * 255)+0.5) << 16) & 0x00FF0000;
   ((GLuint *)packedPixel)[index]|=
     ((GLuint)((shoveComponents[3] * 255)+0.5) << 24) & 0xFF000000;
} /* shove8888rev() */

static void extract1010102(int isSwap,
			   const void *packedPixel,GLfloat extractComponents[])
{
   GLuint uint;

   if (isSwap) {
     uint= __GLU_SWAP_4_BYTES(packedPixel);
   }
   else {
     uint= *(const GLuint *)packedPixel;
   }

   /* 11111111,11000000,00000000,00000000 == 0xffc00000 */
   /* 00000000,00111111,11110000,00000000 == 0x003ff000 */
   /* 00000000,00000000,00001111,11111100 == 0x00000ffc */
   /* 00000000,00000000,00000000,00000011 == 0x00000003 */

   /* 1023 = 2^10-1 */
   extractComponents[0]= (float)((uint & 0xffc00000) >> 22) / 1023.0;
   extractComponents[1]= (float)((uint & 0x003ff000) >> 12) / 1023.0;
   extractComponents[2]= (float)((uint & 0x00000ffc) >>  2) / 1023.0;
   extractComponents[3]= (float)((uint & 0x00000003)	  ) / 3.0;
} /* extract1010102() */

static void shove1010102(const GLfloat shoveComponents[],
			 int index,void *packedPixel)
{
   /* 11111111,11000000,00000000,00000000 == 0xffc00000 */
   /* 00000000,00111111,11110000,00000000 == 0x003ff000 */
   /* 00000000,00000000,00001111,11111100 == 0x00000ffc */
   /* 00000000,00000000,00000000,00000011 == 0x00000003 */

   assert(0.0 <= shoveComponents[0] && shoveComponents[0] <= 1.0);
   assert(0.0 <= shoveComponents[1] && shoveComponents[1] <= 1.0);
   assert(0.0 <= shoveComponents[2] && shoveComponents[2] <= 1.0);
   assert(0.0 <= shoveComponents[3] && shoveComponents[3] <= 1.0);

   /* due to limited precision, need to round before shoving */
   ((GLuint *)packedPixel)[index] =
     ((GLuint)((shoveComponents[0] * 1023)+0.5) << 22) & 0xffc00000;
   ((GLuint *)packedPixel)[index]|=
     ((GLuint)((shoveComponents[1] * 1023)+0.5) << 12) & 0x003ff000;
   ((GLuint *)packedPixel)[index]|=
     ((GLuint)((shoveComponents[2] * 1023)+0.5) <<  2) & 0x00000ffc;
   ((GLuint *)packedPixel)[index]|=
     ((GLuint)((shoveComponents[3] * 3)+0.5)	     ) & 0x00000003;
} /* shove1010102() */

static void extract2101010rev(int isSwap,
			      const void *packedPixel,
			      GLfloat extractComponents[])
{
   GLuint uint;

   if (isSwap) {
     uint= __GLU_SWAP_4_BYTES(packedPixel);
   }
   else {
     uint= *(const GLuint *)packedPixel;
   }

   /* 00000000,00000000,00000011,11111111 == 0x000003FF */
   /* 00000000,00001111,11111100,00000000 == 0x000FFC00 */
   /* 00111111,11110000,00000000,00000000 == 0x3FF00000 */
   /* 11000000,00000000,00000000,00000000 == 0xC0000000 */

   /* 1023 = 2^10-1 */
   extractComponents[0]= (float)((uint & 0x000003FF)	  ) / 1023.0;
   extractComponents[1]= (float)((uint & 0x000FFC00) >> 10) / 1023.0;
   extractComponents[2]= (float)((uint & 0x3FF00000) >> 20) / 1023.0;
   extractComponents[3]= (float)((uint & 0xC0000000) >> 30) / 3.0;
   /* 3 = 2^2-1 */
} /* extract2101010rev() */

static void shove2101010rev(const GLfloat shoveComponents[],
			    int index,void *packedPixel)
{
   /* 00000000,00000000,00000011,11111111 == 0x000003FF */
   /* 00000000,00001111,11111100,00000000 == 0x000FFC00 */
   /* 00111111,11110000,00000000,00000000 == 0x3FF00000 */
   /* 11000000,00000000,00000000,00000000 == 0xC0000000 */

   assert(0.0 <= shoveComponents[0] && shoveComponents[0] <= 1.0);
   assert(0.0 <= shoveComponents[1] && shoveComponents[1] <= 1.0);
   assert(0.0 <= shoveComponents[2] && shoveComponents[2] <= 1.0);
   assert(0.0 <= shoveComponents[3] && shoveComponents[3] <= 1.0);

   /* due to limited precision, need to round before shoving */
   ((GLuint *)packedPixel)[index] =
     ((GLuint)((shoveComponents[0] * 1023)+0.5)      ) & 0x000003FF;
   ((GLuint *)packedPixel)[index]|=
     ((GLuint)((shoveComponents[1] * 1023)+0.5) << 10) & 0x000FFC00;
   ((GLuint *)packedPixel)[index]|=
     ((GLuint)((shoveComponents[2] * 1023)+0.5) << 20) & 0x3FF00000;
   ((GLuint *)packedPixel)[index]|=
     ((GLuint)((shoveComponents[3] * 3)+0.5)	<< 30) & 0xC0000000;
} /* shove2101010rev() */

static void scaleInternalPackedPixel(int components,
				     void (*extractPackedPixel)
				     (int, const void *,GLfloat []),
				     void (*shovePackedPixel)
				     (const GLfloat [], int, void *),
				     GLint widthIn,GLint heightIn,
				     const void *dataIn,
				     GLint widthOut,GLint heightOut,
				     void *dataOut,
				     GLint pixelSizeInBytes,
				     GLint rowSizeInBytes,GLint isSwap)
{
    float convx;
    float convy;
    float percent;

    /* Max components in a format is 4, so... */
    float totals[4];
    float extractTotals[4], extractMoreTotals[4], shoveTotals[4];

    float area;
    int i,j,k,xindex;

    const char *temp, *temp0;
    int outindex;

    int lowx_int, highx_int, lowy_int, highy_int;
    float x_percent, y_percent;
    float lowx_float, highx_float, lowy_float, highy_float;
    float convy_float, convx_float;
    int convy_int, convx_int;
    int l, m;
    const char *left, *right;

    if (widthIn == widthOut*2 && heightIn == heightOut*2) {
	halveImagePackedPixel(components,extractPackedPixel,shovePackedPixel,
			      widthIn, heightIn, dataIn, dataOut,
			      pixelSizeInBytes,rowSizeInBytes,isSwap);
	return;
    }
    convy = (float) heightIn/heightOut;
    convx = (float) widthIn/widthOut;
    convy_int = floor(convy);
    convy_float = convy - convy_int;
    convx_int = floor(convx);
    convx_float = convx - convx_int;

    area = convx * convy;

    lowy_int = 0;
    lowy_float = 0;
    highy_int = convy_int;
    highy_float = convy_float;

    for (i = 0; i < heightOut; i++) {
	lowx_int = 0;
	lowx_float = 0;
	highx_int = convx_int;
	highx_float = convx_float;

	for (j = 0; j < widthOut; j++) {
	    /*
	    ** Ok, now apply box filter to box that goes from (lowx, lowy)
	    ** to (highx, highy) on input data into this pixel on output
	    ** data.
	    */
	    totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

	    /* calculate the value for pixels in the 1st row */
	    xindex = lowx_int*pixelSizeInBytes;
	    if((highy_int>lowy_int) && (highx_int>lowx_int)) {

		y_percent = 1-lowy_float;
		temp = (const char *)dataIn + xindex + lowy_int * rowSizeInBytes;
		percent = y_percent * (1-lowx_float);
#if 0
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
#else
		(*extractPackedPixel)(isSwap,temp,extractTotals);
		for (k = 0; k < components; k++) {
		   totals[k]+= extractTotals[k] * percent;
		}
#endif
		left = temp;
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += pixelSizeInBytes;
#if 0
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				 __GLU_SWAP_2_BYTES(temp_index) * y_percent;
			} else {
			    totals[k] += *(const GLushort*)temp_index * y_percent;
			}
		    }
#else
		    (*extractPackedPixel)(isSwap,temp,extractTotals);
		    for (k = 0; k < components; k++) {
		       totals[k]+= extractTotals[k] * y_percent;
		    }
#endif
		}
		temp += pixelSizeInBytes;
		right = temp;
		percent = y_percent * highx_float;
#if 0
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
#else
		(*extractPackedPixel)(isSwap,temp,extractTotals);
		for (k = 0; k < components; k++) {
		   totals[k]+= extractTotals[k] * percent;
		}
#endif

		/* calculate the value for pixels in the last row */
	        
		y_percent = highy_float;
		percent = y_percent * (1-lowx_float);
		temp = (const char *)dataIn + xindex + highy_int * rowSizeInBytes;
#if 0
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
#else
		(*extractPackedPixel)(isSwap,temp,extractTotals);
		for (k = 0; k < components; k++) {
		   totals[k]+= extractTotals[k] * percent;
		}
#endif
		for(l = lowx_int+1; l < highx_int; l++) {
		    temp += pixelSizeInBytes;
#if 0
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				 __GLU_SWAP_2_BYTES(temp_index) * y_percent;
			} else {
			    totals[k] += *(const GLushort*)temp_index * y_percent;
			}
		    }
#else
		    (*extractPackedPixel)(isSwap,temp,extractTotals);
		    for (k = 0; k < components; k++) {
		       totals[k]+= extractTotals[k] * y_percent;
		    }
#endif

		}
		temp += pixelSizeInBytes;
		percent = y_percent * highx_float;
#if 0
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
#else
		(*extractPackedPixel)(isSwap,temp,extractTotals);
		for (k = 0; k < components; k++) {
		   totals[k]+= extractTotals[k] * percent;
		}
#endif

		/* calculate the value for pixels in the 1st and last column */
		for(m = lowy_int+1; m < highy_int; m++) {
		    left += rowSizeInBytes;
		    right += rowSizeInBytes;
#if 0
		    for (k = 0; k < components;
			 k++, left += element_size, right += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				__GLU_SWAP_2_BYTES(left) * (1-lowx_float) +
				__GLU_SWAP_2_BYTES(right) * highx_float;
			} else {
			    totals[k] += *(const GLushort*)left * (1-lowx_float)
				       + *(const GLushort*)right * highx_float;
			}
		    }
#else
		    (*extractPackedPixel)(isSwap,left,extractTotals);
		    (*extractPackedPixel)(isSwap,right,extractMoreTotals);
		    for (k = 0; k < components; k++) {
		       totals[k]+= (extractTotals[k]*(1-lowx_float) +
				   extractMoreTotals[k]*highx_float);
		    }
#endif
		}
	    } else if (highy_int > lowy_int) {
		x_percent = highx_float - lowx_float;
		percent = (1-lowy_float)*x_percent;
		temp = (const char *)dataIn + xindex + lowy_int*rowSizeInBytes;
#if 0
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
#else
		(*extractPackedPixel)(isSwap,temp,extractTotals);
		for (k = 0; k < components; k++) {
		   totals[k]+= extractTotals[k] * percent;
		}
#endif
		for(m = lowy_int+1; m < highy_int; m++) {
		    temp += rowSizeInBytes;
#if 0
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				__GLU_SWAP_2_BYTES(temp_index) * x_percent;
			} else {
			    totals[k] += *(const GLushort*)temp_index * x_percent;
			}
		    }
#else
		    (*extractPackedPixel)(isSwap,temp,extractTotals);
		    for (k = 0; k < components; k++) {
		       totals[k]+= extractTotals[k] * x_percent;
		    }
#endif
		}
		percent = x_percent * highy_float;
		temp += rowSizeInBytes;
#if 0
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
#else
		(*extractPackedPixel)(isSwap,temp,extractTotals);
		for (k = 0; k < components; k++) {
		   totals[k]+= extractTotals[k] * percent;
		}
#endif
	    } else if (highx_int > lowx_int) {
		y_percent = highy_float - lowy_float;
		percent = (1-lowx_float)*y_percent;
		temp = (const char *)dataIn + xindex + lowy_int*rowSizeInBytes;
#if 0
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
#else
		(*extractPackedPixel)(isSwap,temp,extractTotals);
		for (k = 0; k < components; k++) {
		   totals[k]+= extractTotals[k] * percent;
		}
#endif
		for (l = lowx_int+1; l < highx_int; l++) {
		    temp += pixelSizeInBytes;
#if 0
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] +=
				__GLU_SWAP_2_BYTES(temp_index) * y_percent;
			} else {
			    totals[k] += *(const GLushort*)temp_index * y_percent;
			}
		    }
#else
		(*extractPackedPixel)(isSwap,temp,extractTotals);
		for (k = 0; k < components; k++) {
		   totals[k]+= extractTotals[k] * y_percent;
		}
#endif
		}
		temp += pixelSizeInBytes;
		percent = y_percent * highx_float;
#if 0
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
#else
		(*extractPackedPixel)(isSwap,temp,extractTotals);
		for (k = 0; k < components; k++) {
		   totals[k]+= extractTotals[k] * percent;
		}
#endif
	    } else {
		percent = (highy_float-lowy_float)*(highx_float-lowx_float);
		temp = (const char *)dataIn + xindex + lowy_int * rowSizeInBytes;
#if 0
		for (k = 0, temp_index = temp; k < components;
		     k++, temp_index += element_size) {
		    if (myswap_bytes) {
			totals[k] += __GLU_SWAP_2_BYTES(temp_index) * percent;
		    } else {
			totals[k] += *(const GLushort*)temp_index * percent;
		    }
		}
#else
		(*extractPackedPixel)(isSwap,temp,extractTotals);
		for (k = 0; k < components; k++) {
		   totals[k]+= extractTotals[k] * percent;
		}
#endif
	    }

	    /* this is for the pixels in the body */
	    temp0 = (const char *)dataIn + xindex + pixelSizeInBytes + (lowy_int+1)*rowSizeInBytes;
	    for (m = lowy_int+1; m < highy_int; m++) {
		temp = temp0;
		for(l = lowx_int+1; l < highx_int; l++) {
#if 0
		    for (k = 0, temp_index = temp; k < components;
			 k++, temp_index += element_size) {
			if (myswap_bytes) {
			    totals[k] += __GLU_SWAP_2_BYTES(temp_index);
			} else {
			    totals[k] += *(const GLushort*)temp_index;
			}
		    }
#else
		    (*extractPackedPixel)(isSwap,temp,extractTotals);
		    for (k = 0; k < components; k++) {
		       totals[k]+= extractTotals[k];
		    }
#endif
		    temp += pixelSizeInBytes;
		}
		temp0 += rowSizeInBytes;
	    }

	    outindex = (j + (i * widthOut)); /* * (components == 1) */
#if 0
	    for (k = 0; k < components; k++) {
		dataout[outindex + k] = totals[k]/area;
		/*printf("totals[%d] = %f\n", k, totals[k]);*/
	    }
#else
	    for (k = 0; k < components; k++) {
		shoveTotals[k]= totals[k]/area;
	    }
	    (*shovePackedPixel)(shoveTotals,outindex,(void *)dataOut);
#endif
	    lowx_int = highx_int;
	    lowx_float = highx_float;
	    highx_int += convx_int;
	    highx_float += convx_float;
	    if(highx_float > 1) {
		highx_float -= 1.0;
		highx_int++;
	    }
	}
	lowy_int = highy_int;
	lowy_float = highy_float;
	highy_int += convy_int;
	highy_float += convy_float;
	if(highy_float > 1) {
	    highy_float -= 1.0;
	    highy_int++;
	}
    }

    assert(outindex == (widthOut*heightOut - 1));
} /* scaleInternalPackedPixel() */

/* rowSizeInBytes is at least the width (in bytes) due to padding on
 *  inputs; not always equal. Output NEVER has row padding.
 */
static void halveImagePackedPixel(int components,
				  void (*extractPackedPixel)
				  (int, const void *,GLfloat []),
				  void (*shovePackedPixel)
				  (const GLfloat [],int, void *),
				  GLint width, GLint height,
				  const void *dataIn, void *dataOut,
				  GLint pixelSizeInBytes,
				  GLint rowSizeInBytes, GLint isSwap)
{
   /* handle case where there is only 1 column/row */
   if (width == 1 || height == 1) {
      assert(!(width == 1 && height == 1)); /* can't be 1x1 */
      halve1DimagePackedPixel(components,extractPackedPixel,shovePackedPixel,
			      width,height,dataIn,dataOut,pixelSizeInBytes,
			      rowSizeInBytes,isSwap);
      return;
   }

   {
      int ii, jj;

      int halfWidth= width / 2;
      int halfHeight= height / 2;
      const char *src= (const char *) dataIn;
      int padBytes= rowSizeInBytes - (width*pixelSizeInBytes);
      int outIndex= 0;

      for (ii= 0; ii< halfHeight; ii++) {
	 for (jj= 0; jj< halfWidth; jj++) {
#define BOX4 4
	    float totals[4];	/* 4 is maximum components */
	    float extractTotals[BOX4][4]; /* 4 is maximum components */
	    int cc;

	    (*extractPackedPixel)(isSwap,src,
				  &extractTotals[0][0]);
	    (*extractPackedPixel)(isSwap,(src+pixelSizeInBytes),
				  &extractTotals[1][0]);
	    (*extractPackedPixel)(isSwap,(src+rowSizeInBytes),
				  &extractTotals[2][0]);
	    (*extractPackedPixel)(isSwap,
				  (src+rowSizeInBytes+pixelSizeInBytes),
				  &extractTotals[3][0]);
	    for (cc = 0; cc < components; cc++) {
	       int kk;

	       /* grab 4 pixels to average */
	       totals[cc]= 0.0;
	       /* totals[RED]= extractTotals[0][RED]+extractTotals[1][RED]+
		*	       extractTotals[2][RED]+extractTotals[3][RED];
		* totals[RED]/= 4.0;
		*/
	       for (kk = 0; kk < BOX4; kk++) {
		  totals[cc]+= extractTotals[kk][cc];
	       }
	       totals[cc]/= (float)BOX4;
	    }
	    (*shovePackedPixel)(totals,outIndex,dataOut);

	    outIndex++;
	    /* skip over to next square of 4 */
	    src+= pixelSizeInBytes + pixelSizeInBytes;
	 }
	 /* skip past pad bytes, if any, to get to next row */
	 src+= padBytes;

	 /* src is at beginning of a row here, but it's the second row of
	  * the square block of 4 pixels that we just worked on so we
	  * need to go one more row.
	  * i.e.,
	  *		      OO...
	  *	      here -->OO...
	  *	  but want -->OO...
	  *		      OO...
	  *		      ...
	  */
	 src+= rowSizeInBytes;
      }

      /* both pointers must reach one byte after the end */
      assert(src == &((const char *)dataIn)[rowSizeInBytes*height]);
      assert(outIndex == halfWidth * halfHeight);
   }
} /* halveImagePackedPixel() */

static void halve1DimagePackedPixel(int components,
				    void (*extractPackedPixel)
				    (int, const void *,GLfloat []),
				    void (*shovePackedPixel)
				    (const GLfloat [],int, void *),
				    GLint width, GLint height,
				    const void *dataIn, void *dataOut,
				    GLint pixelSizeInBytes,
				    GLint rowSizeInBytes, GLint isSwap)
{
   int halfWidth= width / 2;
   int halfHeight= height / 2;
   const char *src= (const char *) dataIn;
   int jj;

   assert(width == 1 || height == 1); /* must be 1D */
   assert(width != height);	/* can't be square */

   if (height == 1) {	/* 1 row */
      int outIndex= 0;

      assert(width != 1);	/* widthxheight can't be 1x1 */
      halfHeight= 1;

      /* one horizontal row with possible pad bytes */

      for (jj= 0; jj< halfWidth; jj++) {
#define BOX2 2
	 float totals[4];	/* 4 is maximum components */
	 float extractTotals[BOX2][4]; /* 4 is maximum components */
	 int cc;

	 /* average two at a time, instead of four */
	 (*extractPackedPixel)(isSwap,src,
			       &extractTotals[0][0]);
	 (*extractPackedPixel)(isSwap,(src+pixelSizeInBytes),
			       &extractTotals[1][0]);		        
	 for (cc = 0; cc < components; cc++) {
	    int kk;

	    /* grab 2 pixels to average */
	    totals[cc]= 0.0;
	    /* totals[RED]= extractTotals[0][RED]+extractTotals[1][RED];
	     * totals[RED]/= 2.0;
	     */
	    for (kk = 0; kk < BOX2; kk++) {
	       totals[cc]+= extractTotals[kk][cc];
	    }
	    totals[cc]/= (float)BOX2;
	 }
	 (*shovePackedPixel)(totals,outIndex,dataOut);

	 outIndex++;
	 /* skip over to next group of 2 */
	 src+= pixelSizeInBytes + pixelSizeInBytes;
      }

      {
	 int padBytes= rowSizeInBytes - (width*pixelSizeInBytes);
	 src+= padBytes;	/* for assertion only */
      }
      assert(src == &((const char *)dataIn)[rowSizeInBytes]);
      assert(outIndex == halfWidth * halfHeight);
   }
   else if (width == 1) { /* 1 column */
      int outIndex= 0;

      assert(height != 1);	/* widthxheight can't be 1x1 */
      halfWidth= 1;
      /* one vertical column with possible pad bytes per row */
      /* average two at a time */

      for (jj= 0; jj< halfHeight; jj++) {
#define BOX2 2
	 float totals[4];	/* 4 is maximum components */
	 float extractTotals[BOX2][4]; /* 4 is maximum components */
	 int cc;

	 /* average two at a time, instead of four */
	 (*extractPackedPixel)(isSwap,src,
			       &extractTotals[0][0]);
	 (*extractPackedPixel)(isSwap,(src+rowSizeInBytes),
			       &extractTotals[1][0]);		        
	 for (cc = 0; cc < components; cc++) {
	    int kk;

	    /* grab 2 pixels to average */
	    totals[cc]= 0.0;
	    /* totals[RED]= extractTotals[0][RED]+extractTotals[1][RED];
	     * totals[RED]/= 2.0;
	     */
	    for (kk = 0; kk < BOX2; kk++) {
	       totals[cc]+= extractTotals[kk][cc];
	    }
	    totals[cc]/= (float)BOX2;
	 }
	 (*shovePackedPixel)(totals,outIndex,dataOut);

	 outIndex++;
	 src+= rowSizeInBytes + rowSizeInBytes; /* go to row after next */
      }

      assert(src == &((const char *)dataIn)[rowSizeInBytes*height]);
      assert(outIndex == halfWidth * halfHeight);
   }
} /* halve1DimagePackedPixel() */

/*===========================================================================*/

#ifdef RESOLVE_3D_TEXTURE_SUPPORT
/*
 * This section ensures that GLU 1.3 will load and run on
 * a GL 1.1 implementation. It dynamically resolves the
 * call to glTexImage3D() which might not be available.
 * Or is it might be supported as an extension.
 * Contributed by Gerk Huisma <gerk@five-d.demon.nl>.
 */

typedef void (GLAPIENTRY *TexImage3Dproc)( GLenum target, GLint level,
						 GLenum internalFormat,
						 GLsizei width, GLsizei height,
						 GLsizei depth, GLint border,
						 GLenum format, GLenum type,
						 const GLvoid *pixels );

static TexImage3Dproc pTexImage3D = 0;

#if !defined(_WIN32) && !defined(__WIN32__)
#  include <dlfcn.h>
#  include <sys/types.h>
#else
  WINGDIAPI PROC  WINAPI wglGetProcAddress(LPCSTR);
#endif

static void gluTexImage3D( GLenum target, GLint level,
			   GLenum internalFormat,
			   GLsizei width, GLsizei height,
			   GLsizei depth, GLint border,
			   GLenum format, GLenum type,
			   const GLvoid *pixels )
{
   if (!pTexImage3D) {
#if defined(_WIN32) || defined(__WIN32__)
      pTexImage3D = (TexImage3Dproc) wglGetProcAddress("glTexImage3D");
      if (!pTexImage3D)
	 pTexImage3D = (TexImage3Dproc) wglGetProcAddress("glTexImage3DEXT");
#else
      void *libHandle = dlopen("libgl.so", RTLD_LAZY);
      pTexImage3D = (TexImage3Dproc) dlsym(libHandle, "glTexImage3D" );
      if (!pTexImage3D)
	 pTexImage3D = (TexImage3Dproc) dlsym(libHandle,"glTexImage3DEXT");
      dlclose(libHandle);
#endif
   }

   /* Now call glTexImage3D */
   if (pTexImage3D)
      pTexImage3D(target, level, internalFormat, width, height,
		  depth, border, format, type, pixels);
}

#else

/* Only bind to a GL 1.2 implementation: */
#define gluTexImage3D glTexImage3D

#endif

static GLint imageSize3D(GLint width, GLint height, GLint depth,
			 GLenum format, GLenum type)
{
    int components= elements_per_group(format,type);
    int bytes_per_row=	bytes_per_element(type) * width;

assert(width > 0 && height > 0 && depth > 0);
assert(type != GL_BITMAP);

    return bytes_per_row * height * depth * components;
} /* imageSize3D() */

static void fillImage3D(const PixelStorageModes *psm,
			GLint width, GLint height, GLint depth, GLenum format,
			GLenum type, GLboolean indexFormat,
			const void *userImage, GLushort *newImage)
{
   int myswapBytes;
   int components;
   int groupsPerLine;
   int elementSize;
   int groupSize;
   int rowSize;
   int padding;
   int elementsPerLine;
   int rowsPerImage;
   int imageSize;
   const GLubyte *start, *rowStart, *iter;
   GLushort *iter2;
   int ww, hh, dd, k;

   myswapBytes= psm->unpack_swap_bytes;
   components= elements_per_group(format,type);
   if (psm->unpack_row_length > 0) {
      groupsPerLine= psm->unpack_row_length;
   }
   else {
      groupsPerLine= width;
   }
   elementSize= bytes_per_element(type);
   groupSize= elementSize * components;
   if (elementSize == 1) myswapBytes= 0;

   /* 3dstuff begin */
   if (psm->unpack_image_height > 0) {
      rowsPerImage= psm->unpack_image_height;
   }
   else {
      rowsPerImage= height;
   }
   /* 3dstuff end */

   rowSize= groupsPerLine * groupSize;
   padding= rowSize % psm->unpack_alignment;
   if (padding) {
      rowSize+= psm->unpack_alignment - padding;
   }

   imageSize= rowsPerImage * rowSize; /* 3dstuff */

   start= (const GLubyte *)userImage + psm->unpack_skip_rows * rowSize +
				 psm->unpack_skip_pixels * groupSize +
				 /*3dstuff*/
				 psm->unpack_skip_images * imageSize;
   elementsPerLine = width * components;

   iter2= newImage;
   for (dd= 0; dd < depth; dd++) {
      rowStart= start;

      for (hh= 0; hh < height; hh++) {
	 iter= rowStart;

	 for (ww= 0; ww < elementsPerLine; ww++) {
	    Type_Widget widget;
	    float extractComponents[4];

	    switch(type) {
	    case GL_UNSIGNED_BYTE:
	      if (indexFormat) {
		  *iter2++ = *iter;
	      } else {
		  *iter2++ = (*iter) * 257;
	      }
	      break;
	    case GL_BYTE:
	      if (indexFormat) {
		  *iter2++ = *((const GLbyte *) iter);
	      } else {
		  /* rough approx */
		  *iter2++ = (*((const GLbyte *) iter)) * 516;
	      }
	      break;
	    case GL_UNSIGNED_BYTE_3_3_2:
	      extract332(0,iter,extractComponents);
	      for (k = 0; k < 3; k++) {
		*iter2++ = (GLushort)(extractComponents[k]*65535);
	      }
	      break;
	    case GL_UNSIGNED_BYTE_2_3_3_REV:
	      extract233rev(0,iter,extractComponents);
	      for (k = 0; k < 3; k++) {
		*iter2++ = (GLushort)(extractComponents[k]*65535);
	      }
	      break;
	    case GL_UNSIGNED_SHORT_5_6_5:			        
	      extract565(myswapBytes,iter,extractComponents);
	      for (k = 0; k < 3; k++) {
		*iter2++ = (GLushort)(extractComponents[k]*65535);
	      }
	      break;
	    case GL_UNSIGNED_SHORT_5_6_5_REV:			        
	      extract565rev(myswapBytes,iter,extractComponents);
	      for (k = 0; k < 3; k++) {
		*iter2++ = (GLushort)(extractComponents[k]*65535);
	      }
	      break;
	    case GL_UNSIGNED_SHORT_4_4_4_4:	        
	      extract4444(myswapBytes,iter,extractComponents);
	      for (k = 0; k < 4; k++) {
		*iter2++ = (GLushort)(extractComponents[k]*65535);
	      }
	      break;
	    case GL_UNSIGNED_SHORT_4_4_4_4_REV:         
	      extract4444rev(myswapBytes,iter,extractComponents);
	      for (k = 0; k < 4; k++) {
		*iter2++ = (GLushort)(extractComponents[k]*65535);
	      }
	      break;
	    case GL_UNSIGNED_SHORT_5_5_5_1:	        
	      extract5551(myswapBytes,iter,extractComponents);
	      for (k = 0; k < 4; k++) {
		*iter2++ = (GLushort)(extractComponents[k]*65535);
	      }
	      break;
	    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	      extract1555rev(myswapBytes,iter,extractComponents);
	      for (k = 0; k < 4; k++) {
		*iter2++ = (GLushort)(extractComponents[k]*65535);
	      }
	      break;
	    case GL_UNSIGNED_SHORT:
	    case GL_SHORT:
	      if (myswapBytes) {
		  widget.ub[0] = iter[1];
		  widget.ub[1] = iter[0];
	      } else {
		  widget.ub[0] = iter[0];
		  widget.ub[1] = iter[1];
	      }
	      if (type == GL_SHORT) {
		  if (indexFormat) {
		      *iter2++ = widget.s[0];
		  } else {
		      /* rough approx */
		      *iter2++ = widget.s[0]*2;
		  }
	      } else {
		  *iter2++ = widget.us[0];
	      }
	      break;
	    case GL_UNSIGNED_INT_8_8_8_8:	        
	      extract8888(myswapBytes,iter,extractComponents);
	      for (k = 0; k < 4; k++) {
		*iter2++ = (GLushort)(extractComponents[k]*65535);
	      }
	      break;
	    case GL_UNSIGNED_INT_8_8_8_8_REV:	        
	      extract8888rev(myswapBytes,iter,extractComponents);
	      for (k = 0; k < 4; k++) {
		*iter2++ = (GLushort)(extractComponents[k]*65535);
	      }
	      break;
	    case GL_UNSIGNED_INT_10_10_10_2:	        
	      extract1010102(myswapBytes,iter,extractComponents);
	      for (k = 0; k < 4; k++) {
		*iter2++ = (GLushort)(extractComponents[k]*65535);
	      }
	      break;
	    case GL_UNSIGNED_INT_2_10_10_10_REV:
	      extract2101010rev(myswapBytes,iter,extractComponents);
	      for (k = 0; k < 4; k++) {
		*iter2++ = (GLushort)(extractComponents[k]*65535);
	      }
	      break;
	    case GL_INT:
	    case GL_UNSIGNED_INT:
	    case GL_FLOAT:
	      if (myswapBytes) {
		  widget.ub[0] = iter[3];
		  widget.ub[1] = iter[2];
		  widget.ub[2] = iter[1];
		  widget.ub[3] = iter[0];
	      } else {
		  widget.ub[0] = iter[0];
		  widget.ub[1] = iter[1];
		  widget.ub[2] = iter[2];
		  widget.ub[3] = iter[3];
	      }
	      if (type == GL_FLOAT) {
		  if (indexFormat) {
		      *iter2++ = widget.f;
		  } else {
		      *iter2++ = 65535 * widget.f;
		  }
	      } else if (type == GL_UNSIGNED_INT) {
		  if (indexFormat) {
		      *iter2++ = widget.ui;
		  } else {
		      *iter2++ = widget.ui >> 16;
		  }
	      } else {
		  if (indexFormat) {
		      *iter2++ = widget.i;
		  } else {
		      *iter2++ = widget.i >> 15;
		  }
	      }
	      break;
	    default:
	      assert(0);
	    }

	    iter+= elementSize;
	 } /* for ww */
	 rowStart+= rowSize;

	 iter= rowStart;	/* for assertion purposes */
      } /* for hh */

      start+= imageSize;
   } /* for dd */

   /* iterators should be one byte past end */
   if (!isTypePackedPixel(type)) {
      assert(iter2 == &newImage[width*height*depth*components]);
   }
   else {
      assert(iter2 == &newImage[width*height*depth*
				elements_per_group(format,0)]);
   }
   assert( iter == &((const GLubyte *)userImage)[rowSize*height*depth +
					psm->unpack_skip_rows * rowSize +
					psm->unpack_skip_pixels * groupSize +
					/*3dstuff*/
					psm->unpack_skip_images * imageSize] );
} /* fillImage3D () */

static void scaleInternal3D(GLint components,
			    GLint widthIn, GLint heightIn, GLint depthIn,
			    const GLushort *dataIn,
			    GLint widthOut, GLint heightOut, GLint depthOut,
			    GLushort *dataOut)
{
    float x, lowx, highx, convx, halfconvx;
    float y, lowy, highy, convy, halfconvy;
    float z, lowz, highz, convz, halfconvz;
    float xpercent,ypercent,zpercent;
    float percent;
    /* Max components in a format is 4, so... */
    float totals[4];
    float volume;
    int i,j,d,k,zint,yint,xint,xindex,yindex,zindex;
    int temp;

    convz = (float) depthIn/depthOut;
    convy = (float) heightIn/heightOut;
    convx = (float) widthIn/widthOut;
    halfconvx = convx/2;
    halfconvy = convy/2;
    halfconvz = convz/2;
    for (d = 0; d < depthOut; d++) {
       z = convz * (d+0.5);
       if (depthIn > depthOut) {
	   highz = z + halfconvz;
	   lowz = z - halfconvz;
       } else {
	   highz = z + 0.5;
	   lowz = z - 0.5;
       }
       for (i = 0; i < heightOut; i++) {
	   y = convy * (i+0.5);
	   if (heightIn > heightOut) {
	       highy = y + halfconvy;
	       lowy = y - halfconvy;
	   } else {
	       highy = y + 0.5;
	       lowy = y - 0.5;
	   }
	   for (j = 0; j < widthOut; j++) {
	       x = convx * (j+0.5);
	       if (widthIn > widthOut) {
		   highx = x + halfconvx;
		   lowx = x - halfconvx;
	       } else {
		   highx = x + 0.5;
		   lowx = x - 0.5;
	       }

	       /*
	       ** Ok, now apply box filter to box that goes from (lowx, lowy,
	       ** lowz) to (highx, highy, highz) on input data into this pixel
	       ** on output data.
	       */
	       totals[0] = totals[1] = totals[2] = totals[3] = 0.0;
	       volume = 0.0;

	       z = lowz;
	       zint = floor(z);
	       while (z < highz) {
		  zindex = (zint + depthIn) % depthIn;
		  if (highz < zint+1) {
		      zpercent = highz - z;
		  } else {
		      zpercent = zint+1 - z;
		  }

		  y = lowy;
		  yint = floor(y);
		  while (y < highy) {
		      yindex = (yint + heightIn) % heightIn;
		      if (highy < yint+1) {
			  ypercent = highy - y;
		      } else {
			  ypercent = yint+1 - y;
		      }

		      x = lowx;
		      xint = floor(x);

		      while (x < highx) {
			  xindex = (xint + widthIn) % widthIn;
			  if (highx < xint+1) {
			      xpercent = highx - x;
			  } else {
			      xpercent = xint+1 - x;
			  }

			  percent = xpercent * ypercent * zpercent;
			  volume += percent;

			  temp = (xindex + (yindex*widthIn) +
				  (zindex*widthIn*heightIn)) * components;
			  for (k = 0; k < components; k++) {
			      assert(0 <= (temp+k) &&
				     (temp+k) <
				     (widthIn*heightIn*depthIn*components));
			      totals[k] += dataIn[temp + k] * percent;
			  }

			  xint++;
			  x = xint;
		      } /* while x */

		      yint++;
		      y = yint;
		  } /* while y */

		  zint++;
		  z = zint;
	       } /* while z */

	       temp = (j + (i * widthOut) +
		       (d*widthOut*heightOut)) * components;
	       for (k = 0; k < components; k++) {
		   /* totals[] should be rounded in the case of enlarging an
		    * RGB ramp when the type is 332 or 4444
		    */
		   assert(0 <= (temp+k) &&
			  (temp+k) < (widthOut*heightOut*depthOut*components));
		   dataOut[temp + k] = (totals[k]+0.5)/volume;
	       }
	   } /* for j */
       } /* for i */
    } /* for d */
} /* scaleInternal3D() */

static void emptyImage3D(const PixelStorageModes *psm,
			 GLint width, GLint height, GLint depth,
			 GLenum format, GLenum type, GLboolean indexFormat,
			 const GLushort *oldImage, void *userImage)
{
   int myswapBytes;
   int components;
   int groupsPerLine;
   int elementSize;
   int groupSize;
   int rowSize;
   int padding;
   GLubyte *start, *rowStart, *iter;
   int elementsPerLine;
   const GLushort *iter2;
   int ii, jj, dd, k;
   int rowsPerImage;
   int imageSize;

   myswapBytes= psm->pack_swap_bytes;
   components = elements_per_group(format,type);
   if (psm->pack_row_length > 0) {
      groupsPerLine = psm->pack_row_length;
   }
   else {
      groupsPerLine = width;
   }

   elementSize= bytes_per_element(type);
   groupSize= elementSize * components;
   if (elementSize == 1) myswapBytes= 0;

   /* 3dstuff begin */
   if (psm->pack_image_height > 0) {
      rowsPerImage= psm->pack_image_height;
   }
   else {
      rowsPerImage= height;
   }

   /* 3dstuff end */

   rowSize = groupsPerLine * groupSize;
   padding = rowSize % psm->pack_alignment;
   if (padding) {
      rowSize+= psm->pack_alignment - padding;
   }

   imageSize= rowsPerImage * rowSize; /* 3dstuff */

   start = (GLubyte *)userImage + psm->pack_skip_rows * rowSize +
				  psm->pack_skip_pixels * groupSize +
				  /*3dstuff*/
				  psm->pack_skip_images * imageSize;
   elementsPerLine= width * components;

   iter2 = oldImage;
   for (dd= 0; dd < depth; dd++) {
      rowStart= start;

      for (ii= 0; ii< height; ii++) {
	 iter = rowStart;

	 for (jj = 0; jj < elementsPerLine; jj++) {
	    Type_Widget widget;
	    float shoveComponents[4];

	    switch(type){
	    case GL_UNSIGNED_BYTE:
	      if (indexFormat) {
		  *iter = *iter2++;
	      } else {
		  *iter = *iter2++ >> 8;
	      }
	      break;
	    case GL_BYTE:
	      if (indexFormat) {
		  *((GLbyte *) iter) = *iter2++;
	      } else {
		  *((GLbyte *) iter) = *iter2++ >> 9;
	      }
	      break;
	    case GL_UNSIGNED_BYTE_3_3_2:
	      for (k = 0; k < 3; k++) {
		 shoveComponents[k]= *iter2++ / 65535.0;
	      }
	      shove332(shoveComponents,0,(void *)iter);
	      break;
	    case GL_UNSIGNED_BYTE_2_3_3_REV:
	      for (k = 0; k < 3; k++) {
		 shoveComponents[k]= *iter2++ / 65535.0;
	      }
	      shove233rev(shoveComponents,0,(void *)iter);
	      break;
	    case GL_UNSIGNED_SHORT_5_6_5:	        
	      for (k = 0; k < 3; k++) {
		 shoveComponents[k]= *iter2++ / 65535.0;
	      }
	      shove565(shoveComponents,0,(void *)&widget.us[0]);
	      if (myswapBytes) {
		 iter[0] = widget.ub[1];
		 iter[1] = widget.ub[0];
	      }
	      else {
		 *(GLushort *)iter = widget.us[0];
	      }
	      break;
	    case GL_UNSIGNED_SHORT_5_6_5_REV:	        
	      for (k = 0; k < 3; k++) {
		 shoveComponents[k]= *iter2++ / 65535.0;
	      }
	      shove565rev(shoveComponents,0,(void *)&widget.us[0]);
	      if (myswapBytes) {
		 iter[0] = widget.ub[1];
		 iter[1] = widget.ub[0];
	      }
	      else {
		 *(GLushort *)iter = widget.us[0];
	      }
	      break;
	    case GL_UNSIGNED_SHORT_4_4_4_4:
	      for (k = 0; k < 4; k++) {
		 shoveComponents[k]= *iter2++ / 65535.0;
	      }
	      shove4444(shoveComponents,0,(void *)&widget.us[0]);
	      if (myswapBytes) {
		 iter[0] = widget.ub[1];
		 iter[1] = widget.ub[0];
	      } else {
		 *(GLushort *)iter = widget.us[0];
	      }
	      break;
	    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	      for (k = 0; k < 4; k++) {
		 shoveComponents[k]= *iter2++ / 65535.0;
	      }
	      shove4444rev(shoveComponents,0,(void *)&widget.us[0]);
	      if (myswapBytes) {
		 iter[0] = widget.ub[1];
		 iter[1] = widget.ub[0];
	      } else {
		 *(GLushort *)iter = widget.us[0];
	      }
	      break;
	    case GL_UNSIGNED_SHORT_5_5_5_1:
	      for (k = 0; k < 4; k++) {
		 shoveComponents[k]= *iter2++ / 65535.0;
	      }
	      shove5551(shoveComponents,0,(void *)&widget.us[0]);
	      if (myswapBytes) {
		 iter[0] = widget.ub[1];
		 iter[1] = widget.ub[0];
	      } else {
		 *(GLushort *)iter = widget.us[0];
	      }
	      break;
	    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	      for (k = 0; k < 4; k++) {
		 shoveComponents[k]= *iter2++ / 65535.0;
	      }
	      shove1555rev(shoveComponents,0,(void *)&widget.us[0]);
	      if (myswapBytes) {
		 iter[0] = widget.ub[1];
		 iter[1] = widget.ub[0];
	      } else {
		 *(GLushort *)iter = widget.us[0];
	      }
	      break;
	    case GL_UNSIGNED_SHORT:
	    case GL_SHORT:
	      if (type == GL_SHORT) {
		  if (indexFormat) {
		      widget.s[0] = *iter2++;
		  } else {
		      widget.s[0] = *iter2++ >> 1;
		  }
	      } else {
		  widget.us[0] = *iter2++;
	      }
	      if (myswapBytes) {
		  iter[0] = widget.ub[1];
		  iter[1] = widget.ub[0];
	      } else {
		  iter[0] = widget.ub[0];
		  iter[1] = widget.ub[1];
	      }
	      break;
	    case GL_UNSIGNED_INT_8_8_8_8:
	       for (k = 0; k < 4; k++) {
		  shoveComponents[k]= *iter2++ / 65535.0;
	       }
	       shove8888(shoveComponents,0,(void *)&widget.ui);
	       if (myswapBytes) {
		   iter[3] = widget.ub[0];
		   iter[2] = widget.ub[1];
		   iter[1] = widget.ub[2];
		   iter[0] = widget.ub[3];
	       } else {
		   *(GLuint *)iter= widget.ui;
	       }
	       break;
	    case GL_UNSIGNED_INT_8_8_8_8_REV:
	       for (k = 0; k < 4; k++) {
		  shoveComponents[k]= *iter2++ / 65535.0;
	       }
	       shove8888rev(shoveComponents,0,(void *)&widget.ui);
	       if (myswapBytes) {
		   iter[3] = widget.ub[0];
		   iter[2] = widget.ub[1];
		   iter[1] = widget.ub[2];
		   iter[0] = widget.ub[3];
	       } else {
		   *(GLuint *)iter= widget.ui;
	       }
	       break;
	    case GL_UNSIGNED_INT_10_10_10_2:
	       for (k = 0; k < 4; k++) {
		  shoveComponents[k]= *iter2++ / 65535.0;
	       }
	       shove1010102(shoveComponents,0,(void *)&widget.ui);
	       if (myswapBytes) {
		   iter[3] = widget.ub[0];
		   iter[2] = widget.ub[1];
		   iter[1] = widget.ub[2];
		   iter[0] = widget.ub[3];
	       } else {
		   *(GLuint *)iter= widget.ui;
	       }
	       break;
	    case GL_UNSIGNED_INT_2_10_10_10_REV:
	       for (k = 0; k < 4; k++) {
		  shoveComponents[k]= *iter2++ / 65535.0;
	       }
	       shove2101010rev(shoveComponents,0,(void *)&widget.ui);
	       if (myswapBytes) {
		   iter[3] = widget.ub[0];
		   iter[2] = widget.ub[1];
		   iter[1] = widget.ub[2];
		   iter[0] = widget.ub[3];
	       } else {
		   *(GLuint *)iter= widget.ui;
	       }
	       break;
	    case GL_INT:
	    case GL_UNSIGNED_INT:
	    case GL_FLOAT:
	      if (type == GL_FLOAT) {
		  if (indexFormat) {
		      widget.f = *iter2++;
		  } else {
		      widget.f = *iter2++ / (float) 65535.0;
		  }
	      } else if (type == GL_UNSIGNED_INT) {
		  if (indexFormat) {
		      widget.ui = *iter2++;
		  } else {
		      widget.ui = (unsigned int) *iter2++ * 65537;
		  }
	      } else {
		  if (indexFormat) {
		      widget.i = *iter2++;
		  } else {
		      widget.i = ((unsigned int) *iter2++ * 65537)/2;
		  }
	      }
	      if (myswapBytes) {
		  iter[3] = widget.ub[0];
		  iter[2] = widget.ub[1];
		  iter[1] = widget.ub[2];
		  iter[0] = widget.ub[3];
	      } else {
		  iter[0] = widget.ub[0];
		  iter[1] = widget.ub[1];
		  iter[2] = widget.ub[2];
		  iter[3] = widget.ub[3];
	      }
	      break;
	    default:
	       assert(0);
	    }

	    iter+= elementSize;
	 }  /* for jj */

	 rowStart+= rowSize;
      } /* for ii */

      start+= imageSize;
   } /* for dd */

   /* iterators should be one byte past end */
   if (!isTypePackedPixel(type)) {
      assert(iter2 == &oldImage[width*height*depth*components]);
   }
   else {
      assert(iter2 == &oldImage[width*height*depth*
				elements_per_group(format,0)]);
   }
   assert( iter == &((GLubyte *)userImage)[rowSize*height*depth +
					psm->unpack_skip_rows * rowSize +
					psm->unpack_skip_pixels * groupSize +
					/*3dstuff*/
					psm->unpack_skip_images * imageSize] );
} /* emptyImage3D() */

static
int gluScaleImage3D(GLenum format,
		    GLint widthIn, GLint heightIn, GLint depthIn,
		    GLenum typeIn, const void *dataIn,
		    GLint widthOut, GLint heightOut, GLint depthOut,
		    GLenum typeOut, void *dataOut)
{
   int components;
   GLushort *beforeImage, *afterImage;
   PixelStorageModes psm;

   if (widthIn == 0 || heightIn == 0 || depthIn == 0 ||
       widthOut == 0 || heightOut == 0 || depthOut == 0) {
      return 0;
   }

   if (widthIn < 0 || heightIn < 0 || depthIn < 0 ||
       widthOut < 0 || heightOut < 0 || depthOut < 0) {
      return GLU_INVALID_VALUE;
   }

   if (!legalFormat(format) || !legalType(typeIn) || !legalType(typeOut) ||
       typeIn == GL_BITMAP || typeOut == GL_BITMAP) {
      return GLU_INVALID_ENUM;
   }
   if (!isLegalFormatForPackedPixelType(format, typeIn)) {
      return GLU_INVALID_OPERATION;
   }
   if (!isLegalFormatForPackedPixelType(format, typeOut)) {
      return GLU_INVALID_OPERATION;
   }

   beforeImage = malloc(imageSize3D(widthIn, heightIn, depthIn, format,
				    GL_UNSIGNED_SHORT));
   afterImage = malloc(imageSize3D(widthOut, heightOut, depthOut, format,
				   GL_UNSIGNED_SHORT));
   if (beforeImage == NULL || afterImage == NULL) {
       free(beforeImage);
       free(afterImage);
       return GLU_OUT_OF_MEMORY;
   }
   retrieveStoreModes3D(&psm);

   fillImage3D(&psm,widthIn,heightIn,depthIn,format,typeIn, is_index(format),
	       dataIn, beforeImage);
   components = elements_per_group(format,0);
   scaleInternal3D(components,widthIn,heightIn,depthIn,beforeImage,
		   widthOut,heightOut,depthOut,afterImage);
   emptyImage3D(&psm,widthOut,heightOut,depthOut,format,typeOut,
		is_index(format),afterImage, dataOut);
   free((void *) beforeImage);
   free((void *) afterImage);

   return 0;
} /* gluScaleImage3D() */


static void closestFit3D(GLenum target, GLint width, GLint height, GLint depth,
			 GLint internalFormat, GLenum format, GLenum type,
			 GLint *newWidth, GLint *newHeight, GLint *newDepth)
{
   GLint widthPowerOf2= nearestPower(width);
   GLint heightPowerOf2= nearestPower(height);	        
   GLint depthPowerOf2= nearestPower(depth);
   GLint proxyWidth;

   do {
      /* compute level 1 width & height & depth, clamping each at 1 */
      GLint widthAtLevelOne= (widthPowerOf2 > 1) ?
			      widthPowerOf2 >> 1 :
			      widthPowerOf2;
      GLint heightAtLevelOne= (heightPowerOf2 > 1) ?
			       heightPowerOf2 >> 1 :
			       heightPowerOf2;
      GLint depthAtLevelOne= (depthPowerOf2 > 1) ?
			      depthPowerOf2 >> 1 :
			      depthPowerOf2;
      GLenum proxyTarget = GL_PROXY_TEXTURE_3D;
      assert(widthAtLevelOne > 0);
      assert(heightAtLevelOne > 0);
      assert(depthAtLevelOne > 0);

      /* does width x height x depth at level 1 & all their mipmaps fit? */
      assert(target == GL_TEXTURE_3D || target == GL_PROXY_TEXTURE_3D);
      gluTexImage3D(proxyTarget, 1, /* must be non-zero */
                    internalFormat,
                    widthAtLevelOne,heightAtLevelOne,depthAtLevelOne,
                    0,format,type,NULL);
      glGetTexLevelParameteriv(proxyTarget, 1,GL_TEXTURE_WIDTH,&proxyWidth);
      /* does it fit??? */
      if (proxyWidth == 0) { /* nope, so try again with these sizes */
	 if (widthPowerOf2 == 1 && heightPowerOf2 == 1 &&
	     depthPowerOf2 == 1) {
	    *newWidth= *newHeight= *newDepth= 1; /* must fit 1x1x1 texture */
	    return;
	 }
	 widthPowerOf2= widthAtLevelOne;
	 heightPowerOf2= heightAtLevelOne;
	 depthPowerOf2= depthAtLevelOne;
      }
      /* else it does fit */
   } while (proxyWidth == 0);
   /* loop must terminate! */

   /* return the width & height at level 0 that fits */
   *newWidth= widthPowerOf2;
   *newHeight= heightPowerOf2;
   *newDepth= depthPowerOf2;
/*printf("Proxy Textures\n");*/
} /* closestFit3D() */

static void halveImagePackedPixelSlice(int components,
				       void (*extractPackedPixel)
				       (int, const void *,GLfloat []),
				       void (*shovePackedPixel)
				       (const GLfloat [],int, void *),
				       GLint width, GLint height, GLint depth,
				       const void *dataIn, void *dataOut,
				       GLint pixelSizeInBytes,
				       GLint rowSizeInBytes,
				       GLint imageSizeInBytes,
				       GLint isSwap)
{
   int ii, jj;
   int halfWidth= width / 2;
   int halfHeight= height / 2;
   int halfDepth= depth / 2;
   const char *src= (const char *)dataIn;
   int outIndex= 0;

   assert((width == 1 || height == 1) && depth >= 2);

   if (width == height) {	/* a 1-pixel column viewed from top */
      assert(width == 1 && height == 1);
      assert(depth >= 2);

      for (ii= 0; ii< halfDepth; ii++) {
	 float totals[4];
	 float extractTotals[BOX2][4];
	 int cc;

	 (*extractPackedPixel)(isSwap,src,&extractTotals[0][0]);
	 (*extractPackedPixel)(isSwap,(src+imageSizeInBytes),
			       &extractTotals[1][0]);
	 for (cc = 0; cc < components; cc++) {
	    int kk;

	    /* average 2 pixels since only a column */
	    totals[cc]= 0.0;
	    /* totals[RED]= extractTotals[0][RED]+extractTotals[1][RED];
	     * totals[RED]/= 2.0;
	     */
	    for (kk = 0; kk < BOX2; kk++) {
	      totals[cc]+= extractTotals[kk][cc];
	    }
	    totals[cc]/= (float)BOX2;
	 } /* for cc */
        
	 (*shovePackedPixel)(totals,outIndex,dataOut);
	 outIndex++;
	 /* skip over to next group of 2 */
	 src+= imageSizeInBytes + imageSizeInBytes;
      } /* for ii */
   }
   else if (height == 1) {	/* horizontal slice viewed from top */
      assert(width != 1);

      for (ii= 0; ii< halfDepth; ii++) {
	 for (jj= 0; jj< halfWidth; jj++) {
	     float totals[4];
	     float extractTotals[BOX4][4];
	     int cc;

	     (*extractPackedPixel)(isSwap,src,
				   &extractTotals[0][0]);
	     (*extractPackedPixel)(isSwap,(src+pixelSizeInBytes),
				   &extractTotals[1][0]);
	     (*extractPackedPixel)(isSwap,(src+imageSizeInBytes),
				   &extractTotals[2][0]);
	     (*extractPackedPixel)(isSwap,
				   (src+imageSizeInBytes+pixelSizeInBytes),
				   &extractTotals[3][0]);
	     for (cc = 0; cc < components; cc++) {
		int kk;

		/* grab 4 pixels to average */
		totals[cc]= 0.0;
		/* totals[RED]= extractTotals[0][RED]+extractTotals[1][RED]+
		 *		extractTotals[2][RED]+extractTotals[3][RED];
		 * totals[RED]/= 4.0;
		 */
		for (kk = 0; kk < BOX4; kk++) {
		   totals[cc]+= extractTotals[kk][cc];
		}
		totals[cc]/= (float)BOX4;
	     }
	     (*shovePackedPixel)(totals,outIndex,dataOut);

	     outIndex++;
	     /* skip over to next horizontal square of 4 */
	     src+= imageSizeInBytes + imageSizeInBytes;
	 }
      }

      /* assert() */
   }
   else if (width == 1) {	/* vertical slice viewed from top */
      assert(height != 1);

      for (ii= 0; ii< halfDepth; ii++) {
	 for (jj= 0; jj< halfHeight; jj++) {
	    float totals[4];
	    float extractTotals[BOX4][4];
	    int cc;

	    (*extractPackedPixel)(isSwap,src,
				  &extractTotals[0][0]);
	    (*extractPackedPixel)(isSwap,(src+rowSizeInBytes),
				  &extractTotals[1][0]);
	    (*extractPackedPixel)(isSwap,(src+imageSizeInBytes),
				  &extractTotals[2][0]);
	    (*extractPackedPixel)(isSwap,
				  (src+imageSizeInBytes+rowSizeInBytes),
				  &extractTotals[3][0]);
	    for (cc = 0; cc < components; cc++) {
	       int kk;

	       /* grab 4 pixels to average */
	       totals[cc]= 0.0;
	       /* totals[RED]= extractTotals[0][RED]+extractTotals[1][RED]+
		*	       extractTotals[2][RED]+extractTotals[3][RED];
		* totals[RED]/= 4.0;
		*/
	       for (kk = 0; kk < BOX4; kk++) {
		  totals[cc]+= extractTotals[kk][cc];
	       }
	       totals[cc]/= (float)BOX4;
	    }
	    (*shovePackedPixel)(totals,outIndex,dataOut);

	    outIndex++;

	    /* skip over to next vertical square of 4 */
	    src+= imageSizeInBytes + imageSizeInBytes;
	 }
      }
      /* assert() */
   }

} /* halveImagePackedPixelSlice() */

static void halveImagePackedPixel3D(int components,
				    void (*extractPackedPixel)
				    (int, const void *,GLfloat []),
				    void (*shovePackedPixel)
				    (const GLfloat [],int, void *),
				    GLint width, GLint height, GLint depth,
				    const void *dataIn, void *dataOut,
				    GLint pixelSizeInBytes,
				    GLint rowSizeInBytes,
				    GLint imageSizeInBytes,
				    GLint isSwap)
{
   if (depth == 1) {
      assert(1 <= width && 1 <= height);

      halveImagePackedPixel(components,extractPackedPixel,shovePackedPixel,
			    width,height,dataIn,dataOut,pixelSizeInBytes,
			    rowSizeInBytes,isSwap);
      return;
   }
   /* a horizontal or vertical slice viewed from top */
   else if (width == 1 || height == 1) {
      assert(1 <= depth);

      halveImagePackedPixelSlice(components,
				 extractPackedPixel,shovePackedPixel,
				 width, height, depth, dataIn, dataOut,
				 pixelSizeInBytes, rowSizeInBytes,
				 imageSizeInBytes, isSwap);
      return;
   }
   {
      int ii, jj, dd;

      int halfWidth= width / 2;
      int halfHeight= height / 2;
      int halfDepth= depth / 2;
      const char *src= (const char *) dataIn;
      int padBytes= rowSizeInBytes - (width*pixelSizeInBytes);
      int outIndex= 0;

      for (dd= 0; dd < halfDepth; dd++) {
	 for (ii= 0; ii< halfHeight; ii++) {
	    for (jj= 0; jj< halfWidth; jj++) {
#define BOX8 8
	       float totals[4]; /* 4 is maximum components */
	       float extractTotals[BOX8][4]; /* 4 is maximum components */
	       int cc;

	       (*extractPackedPixel)(isSwap,src,
				     &extractTotals[0][0]);
	       (*extractPackedPixel)(isSwap,(src+pixelSizeInBytes),
				     &extractTotals[1][0]);
	       (*extractPackedPixel)(isSwap,(src+rowSizeInBytes),
				     &extractTotals[2][0]);
	       (*extractPackedPixel)(isSwap,
				     (src+rowSizeInBytes+pixelSizeInBytes),
				     &extractTotals[3][0]);

	       (*extractPackedPixel)(isSwap,(src+imageSizeInBytes),
				     &extractTotals[4][0]);
	       (*extractPackedPixel)(isSwap,(src+pixelSizeInBytes+imageSizeInBytes),
				     &extractTotals[5][0]);
	       (*extractPackedPixel)(isSwap,(src+rowSizeInBytes+imageSizeInBytes),
				     &extractTotals[6][0]);
	       (*extractPackedPixel)(isSwap,
				     (src+rowSizeInBytes+pixelSizeInBytes+imageSizeInBytes),
				     &extractTotals[7][0]);
	       for (cc = 0; cc < components; cc++) {
		  int kk;

		  /* grab 8 pixels to average */
		  totals[cc]= 0.0;
		  /* totals[RED]= extractTotals[0][RED]+extractTotals[1][RED]+
		   *		  extractTotals[2][RED]+extractTotals[3][RED]+
		   *		  extractTotals[4][RED]+extractTotals[5][RED]+
		   *		  extractTotals[6][RED]+extractTotals[7][RED];
		   * totals[RED]/= 8.0;
		   */
		  for (kk = 0; kk < BOX8; kk++) {
		     totals[cc]+= extractTotals[kk][cc];
		  }
		  totals[cc]/= (float)BOX8;
	       }
	       (*shovePackedPixel)(totals,outIndex,dataOut);

	       outIndex++;
	       /* skip over to next square of 4 */
	       src+= pixelSizeInBytes + pixelSizeInBytes;
	    }
	    /* skip past pad bytes, if any, to get to next row */
	    src+= padBytes;

	    /* src is at beginning of a row here, but it's the second row of
	     * the square block of 4 pixels that we just worked on so we
	     * need to go one more row.
	     * i.e.,
	     *			 OO...
	     *		 here -->OO...
	     *	     but want -->OO...
	     *			 OO...
	     *			 ...
	     */
	    src+= rowSizeInBytes;
	 }

	 src+= imageSizeInBytes;
      } /* for dd */

      /* both pointers must reach one byte after the end */
      assert(src == &((const char *)dataIn)[rowSizeInBytes*height*depth]);
      assert(outIndex == halfWidth * halfHeight * halfDepth);
   } /* for dd */

} /* halveImagePackedPixel3D() */

static int gluBuild3DMipmapLevelsCore(GLenum target, GLint internalFormat,
				      GLsizei width,
				      GLsizei height,
				      GLsizei depth,
				      GLsizei widthPowerOf2,
				      GLsizei heightPowerOf2,
				      GLsizei depthPowerOf2,
				      GLenum format, GLenum type,
				      GLint userLevel,
				      GLint baseLevel,GLint maxLevel,
				      const void *data)
{
   GLint newWidth, newHeight, newDepth;
   GLint level, levels;
   const void *usersImage;
   void *srcImage, *dstImage;
   __GLU_INIT_SWAP_IMAGE;
   GLint memReq;
   GLint cmpts;

   GLint myswapBytes, groupsPerLine, elementSize, groupSize;
   GLint rowsPerImage, imageSize;
   GLint rowSize, padding;
   PixelStorageModes psm;

   assert(checkMipmapArgs(internalFormat,format,type) == 0);
   assert(width >= 1 && height >= 1 && depth >= 1);
   assert(type != GL_BITMAP);

   srcImage = dstImage = NULL;

   newWidth= widthPowerOf2;
   newHeight= heightPowerOf2;
   newDepth= depthPowerOf2;
   levels = computeLog(newWidth);
   level = computeLog(newHeight);
   if (level > levels) levels=level;
   level = computeLog(newDepth);
   if (level > levels) levels=level;

   levels+= userLevel;

   retrieveStoreModes3D(&psm);
   myswapBytes = psm.unpack_swap_bytes;
   cmpts = elements_per_group(format,type);
   if (psm.unpack_row_length > 0) {
       groupsPerLine = psm.unpack_row_length;
   } else {
       groupsPerLine = width;
   }

   elementSize = bytes_per_element(type);
   groupSize = elementSize * cmpts;
   if (elementSize == 1) myswapBytes = 0;

   /* 3dstuff begin */
   if (psm.unpack_image_height > 0) {
      rowsPerImage= psm.unpack_image_height;
   }
   else {
      rowsPerImage= height;
   }

   /* 3dstuff end */
   rowSize = groupsPerLine * groupSize;
   padding = (rowSize % psm.unpack_alignment);
   if (padding) {
       rowSize += psm.unpack_alignment - padding;
   }

   imageSize= rowsPerImage * rowSize; /* 3dstuff */

   usersImage = (const GLubyte *)data + psm.unpack_skip_rows * rowSize +
				  psm.unpack_skip_pixels * groupSize +
				  /* 3dstuff */
				  psm.unpack_skip_images * imageSize;

   glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
   glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0);
   glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);

   level = userLevel;

   if (width == newWidth && height == newHeight && depth == newDepth) {
       /* Use usersImage for level userLevel */
       if (baseLevel <= level && level <= maxLevel) {
	  gluTexImage3D(target, level, internalFormat, width,
		       height, depth, 0, format, type,
		       usersImage);
       }
       if(levels == 0) { /* we're done. clean up and return */
	 glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
	 glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
	 glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
	 glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
	 glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
	 glPixelStorei(GL_UNPACK_SKIP_IMAGES, psm.unpack_skip_images);
	 glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, psm.unpack_image_height);
	 return 0;
       }
       {
	  int nextWidth= newWidth/2;
	  int nextHeight= newHeight/2;
	  int nextDepth= newDepth/2;

	  /* clamp to 1 */
	  if (nextWidth < 1) nextWidth= 1;
	  if (nextHeight < 1) nextHeight= 1;
	  if (nextDepth < 1) nextDepth= 1;      
       memReq = imageSize3D(nextWidth, nextHeight, nextDepth, format, type);
       }
       switch(type) {
       case GL_UNSIGNED_BYTE:
	 dstImage = (GLubyte *)malloc(memReq);
	 break;
       case GL_BYTE:
	 dstImage = (GLbyte *)malloc(memReq);
	 break;
       case GL_UNSIGNED_SHORT:
	 dstImage = (GLushort *)malloc(memReq);
	 break;
       case GL_SHORT:
	 dstImage = (GLshort *)malloc(memReq);
	 break;
       case GL_UNSIGNED_INT:
	 dstImage = (GLuint *)malloc(memReq);
	 break;
       case GL_INT:
	 dstImage = (GLint *)malloc(memReq);
	 break;
       case GL_FLOAT:
	 dstImage = (GLfloat *)malloc(memReq);
	 break;
       case GL_UNSIGNED_BYTE_3_3_2:
       case GL_UNSIGNED_BYTE_2_3_3_REV:
	 dstImage = (GLubyte *)malloc(memReq);
	 break;
       case GL_UNSIGNED_SHORT_5_6_5:
       case GL_UNSIGNED_SHORT_5_6_5_REV:
       case GL_UNSIGNED_SHORT_4_4_4_4:
       case GL_UNSIGNED_SHORT_4_4_4_4_REV:
       case GL_UNSIGNED_SHORT_5_5_5_1:
       case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	 dstImage = (GLushort *)malloc(memReq);
	 break;
       case GL_UNSIGNED_INT_8_8_8_8:
       case GL_UNSIGNED_INT_8_8_8_8_REV:
       case GL_UNSIGNED_INT_10_10_10_2:
       case GL_UNSIGNED_INT_2_10_10_10_REV:
	 dstImage = (GLuint *)malloc(memReq);   
	 break;
       default:
	 return GLU_INVALID_ENUM; /* assertion */
       }
       if (dstImage == NULL) {
	 glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
	 glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
	 glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
	 glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
	 glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
	 glPixelStorei(GL_UNPACK_SKIP_IMAGES, psm.unpack_skip_images);
	 glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, psm.unpack_image_height);
	 return GLU_OUT_OF_MEMORY;
       }
       else
	 switch(type) {
	 case GL_UNSIGNED_BYTE:
	   if (depth > 1) {
	     halveImage3D(cmpts,extractUbyte,shoveUbyte,
			  width,height,depth,
			  usersImage,dstImage,elementSize,groupSize,rowSize,
			  imageSize,myswapBytes);
	   }
	   else {
	     halveImage_ubyte(cmpts,width,height,usersImage,dstImage,
			      elementSize,rowSize,groupSize);
	   }
	   break;
	 case GL_BYTE:
	   if (depth > 1) {
	   halveImage3D(cmpts,extractSbyte,shoveSbyte,
			width,height,depth,
			usersImage,dstImage,elementSize,groupSize,rowSize,
			imageSize,myswapBytes);
	   }
	   else {
	     halveImage_byte(cmpts,width,height,usersImage,dstImage,
			     elementSize,rowSize,groupSize);
	   }
	   break;
	 case GL_UNSIGNED_SHORT:
	   if (depth > 1) {
	   halveImage3D(cmpts,extractUshort,shoveUshort,
			width,height,depth,
			usersImage,dstImage,elementSize,groupSize,rowSize,
			imageSize,myswapBytes);
	   }
	   else {
	     halveImage_ushort(cmpts,width,height,usersImage,dstImage,
			       elementSize,rowSize,groupSize,myswapBytes);
	   }
	   break;
	 case GL_SHORT:
	   if (depth > 1) {
	   halveImage3D(cmpts,extractSshort,shoveSshort,
			width,height,depth,
			usersImage,dstImage,elementSize,groupSize,rowSize,
			imageSize,myswapBytes);
	   }
	   else {
	     halveImage_short(cmpts,width,height,usersImage,dstImage,
			      elementSize,rowSize,groupSize,myswapBytes);
	   }
	   break;
	 case GL_UNSIGNED_INT:
	   if (depth > 1) {
	   halveImage3D(cmpts,extractUint,shoveUint,
			width,height,depth,
			usersImage,dstImage,elementSize,groupSize,rowSize,
			imageSize,myswapBytes);
	   }
	   else {
	     halveImage_uint(cmpts,width,height,usersImage,dstImage,
			     elementSize,rowSize,groupSize,myswapBytes);
	   }
	   break;
	 case GL_INT:
	   if (depth > 1) {
	   halveImage3D(cmpts,extractSint,shoveSint,
			width,height,depth,
			usersImage,dstImage,elementSize,groupSize,rowSize,
			imageSize,myswapBytes);
	   }
	   else {
	     halveImage_int(cmpts,width,height,usersImage,dstImage,
			    elementSize,rowSize,groupSize,myswapBytes);
	   }
	   break;
	 case GL_FLOAT:
	   if (depth > 1 ) {
	   halveImage3D(cmpts,extractFloat,shoveFloat,
			width,height,depth,
			usersImage,dstImage,elementSize,groupSize,rowSize,
			imageSize,myswapBytes);
	   }
	   else {
	     halveImage_float(cmpts,width,height,usersImage,dstImage,
			      elementSize,rowSize,groupSize,myswapBytes);
	   }
	   break;
	 case GL_UNSIGNED_BYTE_3_3_2:
	   assert(format == GL_RGB);
	   halveImagePackedPixel3D(3,extract332,shove332,
				   width,height,depth,usersImage,dstImage,
				   elementSize,rowSize,imageSize,myswapBytes);
	   break;
	 case GL_UNSIGNED_BYTE_2_3_3_REV:
	   assert(format == GL_RGB);
	   halveImagePackedPixel3D(3,extract233rev,shove233rev,
				   width,height,depth,usersImage,dstImage,
				   elementSize,rowSize,imageSize,myswapBytes);
	   break;
	 case GL_UNSIGNED_SHORT_5_6_5:
	   halveImagePackedPixel3D(3,extract565,shove565,
				   width,height,depth,usersImage,dstImage,
				   elementSize,rowSize,imageSize,myswapBytes);
	   break;
	 case GL_UNSIGNED_SHORT_5_6_5_REV:
	   halveImagePackedPixel3D(3,extract565rev,shove565rev,
				   width,height,depth,usersImage,dstImage,
				   elementSize,rowSize,imageSize,myswapBytes);
	   break;
	 case GL_UNSIGNED_SHORT_4_4_4_4:
	   halveImagePackedPixel3D(4,extract4444,shove4444,
				   width,height,depth,usersImage,dstImage,
				   elementSize,rowSize,imageSize,myswapBytes);
	   break;
	 case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	   halveImagePackedPixel3D(4,extract4444rev,shove4444rev,
				   width,height,depth,usersImage,dstImage,
				   elementSize,rowSize,imageSize,myswapBytes);
	   break;
	 case GL_UNSIGNED_SHORT_5_5_5_1:
	   halveImagePackedPixel3D(4,extract5551,shove5551,
				   width,height,depth,usersImage,dstImage,
				   elementSize,rowSize,imageSize,myswapBytes);
	   break;
	 case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	   halveImagePackedPixel3D(4,extract1555rev,shove1555rev,
				   width,height,depth,usersImage,dstImage,
				   elementSize,rowSize,imageSize,myswapBytes);
	   break;
	 case GL_UNSIGNED_INT_8_8_8_8:
	   halveImagePackedPixel3D(4,extract8888,shove8888,
				   width,height,depth,usersImage,dstImage,
				   elementSize,rowSize,imageSize,myswapBytes);
	   break;
	 case GL_UNSIGNED_INT_8_8_8_8_REV:
	   halveImagePackedPixel3D(4,extract8888rev,shove8888rev,
				   width,height,depth,usersImage,dstImage,
				   elementSize,rowSize,imageSize,myswapBytes);
	   break;
	 case GL_UNSIGNED_INT_10_10_10_2:
	   halveImagePackedPixel3D(4,extract1010102,shove1010102,
				   width,height,depth,usersImage,dstImage,
				   elementSize,rowSize,imageSize,myswapBytes);
	   break;
	 case GL_UNSIGNED_INT_2_10_10_10_REV:
	   halveImagePackedPixel3D(4,extract2101010rev,shove2101010rev,
				   width,height,depth,usersImage,dstImage,
				   elementSize,rowSize,imageSize,myswapBytes);
	   break;
	 default:
	   assert(0);
	   break;
	 }
       newWidth = width/2;
       newHeight = height/2;
       newDepth = depth/2;
       /* clamp to 1 */
       if (newWidth < 1) newWidth= 1;
       if (newHeight < 1) newHeight= 1;
       if (newDepth < 1) newDepth= 1;

       myswapBytes = 0;
       rowSize = newWidth * groupSize;
       imageSize= rowSize * newHeight; /* 3dstuff */
       memReq = imageSize3D(newWidth, newHeight, newDepth, format, type);
       /* Swap srcImage and dstImage */
       __GLU_SWAP_IMAGE(srcImage,dstImage);
       switch(type) {
       case GL_UNSIGNED_BYTE:
	 dstImage = (GLubyte *)malloc(memReq);
	 break;
       case GL_BYTE:
	 dstImage = (GLbyte *)malloc(memReq);
	 break;
       case GL_UNSIGNED_SHORT:
	 dstImage = (GLushort *)malloc(memReq);
	 break;
       case GL_SHORT:
	 dstImage = (GLshort *)malloc(memReq);
	 break;
       case GL_UNSIGNED_INT:
	 dstImage = (GLuint *)malloc(memReq);
	 break;
       case GL_INT:
	 dstImage = (GLint *)malloc(memReq);
	 break;
       case GL_FLOAT:
	 dstImage = (GLfloat *)malloc(memReq);
	 break;
       case GL_UNSIGNED_BYTE_3_3_2:
       case GL_UNSIGNED_BYTE_2_3_3_REV:
	 dstImage = (GLubyte *)malloc(memReq);
	 break;
       case GL_UNSIGNED_SHORT_5_6_5:
       case GL_UNSIGNED_SHORT_5_6_5_REV:
       case GL_UNSIGNED_SHORT_4_4_4_4:
       case GL_UNSIGNED_SHORT_4_4_4_4_REV:
       case GL_UNSIGNED_SHORT_5_5_5_1:
       case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	 dstImage = (GLushort *)malloc(memReq);
	 break;
       case GL_UNSIGNED_INT_8_8_8_8:
       case GL_UNSIGNED_INT_8_8_8_8_REV:
       case GL_UNSIGNED_INT_10_10_10_2:
       case GL_UNSIGNED_INT_2_10_10_10_REV:
	 dstImage = (GLuint *)malloc(memReq);
	 break;
       default:
	 return GLU_INVALID_ENUM; /* assertion */
       }
       if (dstImage == NULL) {
	 glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
	 glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
	 glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
	 glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
	 glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
	 glPixelStorei(GL_UNPACK_SKIP_IMAGES, psm.unpack_skip_images);
	 glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, psm.unpack_image_height);
	 free(srcImage);
	 return GLU_OUT_OF_MEMORY;
       }
       /* level userLevel+1 is in srcImage; level userLevel already saved */
       level = userLevel+1;
   } else {/* user's image is *not* nice power-of-2 sized square */
       memReq = imageSize3D(newWidth, newHeight, newDepth, format, type);
       switch(type) {
	   case GL_UNSIGNED_BYTE:
	       dstImage = (GLubyte *)malloc(memReq);
	       break;
	   case GL_BYTE:
	       dstImage = (GLbyte *)malloc(memReq);
	       break;
	   case GL_UNSIGNED_SHORT:
	       dstImage = (GLushort *)malloc(memReq);
	       break;
	   case GL_SHORT:
	       dstImage = (GLshort *)malloc(memReq);
	       break;
	   case GL_UNSIGNED_INT:
	       dstImage = (GLuint *)malloc(memReq);
	       break;
	   case GL_INT:
	       dstImage = (GLint *)malloc(memReq);
	       break;
	   case GL_FLOAT:
	       dstImage = (GLfloat *)malloc(memReq);
	       break;
	   case GL_UNSIGNED_BYTE_3_3_2:
	   case GL_UNSIGNED_BYTE_2_3_3_REV:
	       dstImage = (GLubyte *)malloc(memReq);
	       break;
	   case GL_UNSIGNED_SHORT_5_6_5:
	   case GL_UNSIGNED_SHORT_5_6_5_REV:
	   case GL_UNSIGNED_SHORT_4_4_4_4:
	   case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	   case GL_UNSIGNED_SHORT_5_5_5_1:
	   case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	       dstImage = (GLushort *)malloc(memReq);
	       break;
	   case GL_UNSIGNED_INT_8_8_8_8:
	   case GL_UNSIGNED_INT_8_8_8_8_REV:
	   case GL_UNSIGNED_INT_10_10_10_2:
	   case GL_UNSIGNED_INT_2_10_10_10_REV:
	       dstImage = (GLuint *)malloc(memReq);
	       break;
	   default:
	       return GLU_INVALID_ENUM; /* assertion */
       }

       if (dstImage == NULL) {
	   glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
	   glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
	   glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
	   glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
	   glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
	   glPixelStorei(GL_UNPACK_SKIP_IMAGES, psm.unpack_skip_images);
	   glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, psm.unpack_image_height);
	   return GLU_OUT_OF_MEMORY;
       }
       /*printf("Build3DMipmaps(): ScaleImage3D %d %d %d->%d %d %d\n",
       width,height,depth,newWidth,newHeight,newDepth);*/

       gluScaleImage3D(format, width, height, depth, type, usersImage,
		       newWidth, newHeight, newDepth, type, dstImage);

       myswapBytes = 0;
       rowSize = newWidth * groupSize;
       imageSize = rowSize * newHeight; /* 3dstuff */
       /* Swap dstImage and srcImage */
       __GLU_SWAP_IMAGE(srcImage,dstImage);

       if(levels != 0) { /* use as little memory as possible */
	 {
	    int nextWidth= newWidth/2;
	    int nextHeight= newHeight/2;
	    int nextDepth= newDepth/2;
	    if (nextWidth < 1) nextWidth= 1;
	    if (nextHeight < 1) nextHeight= 1;  
	    if (nextDepth < 1) nextDepth= 1;    

	 memReq = imageSize3D(nextWidth, nextHeight, nextDepth, format, type);
	 }
	 switch(type) {
	 case GL_UNSIGNED_BYTE:
	   dstImage = (GLubyte *)malloc(memReq);
	   break;
	 case GL_BYTE:
	   dstImage = (GLbyte *)malloc(memReq);
	   break;
	 case GL_UNSIGNED_SHORT:
	   dstImage = (GLushort *)malloc(memReq);
	   break;
	 case GL_SHORT:
	   dstImage = (GLshort *)malloc(memReq);
	   break;
	 case GL_UNSIGNED_INT:
	   dstImage = (GLuint *)malloc(memReq);
	   break;
	 case GL_INT:
	   dstImage = (GLint *)malloc(memReq);
	   break;
	 case GL_FLOAT:
	   dstImage = (GLfloat *)malloc(memReq);
	   break;
	 case GL_UNSIGNED_BYTE_3_3_2:
	 case GL_UNSIGNED_BYTE_2_3_3_REV:
	   dstImage = (GLubyte *)malloc(memReq);
	   break;
	 case GL_UNSIGNED_SHORT_5_6_5:
	 case GL_UNSIGNED_SHORT_5_6_5_REV:
	 case GL_UNSIGNED_SHORT_4_4_4_4:
	 case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	 case GL_UNSIGNED_SHORT_5_5_5_1:
	 case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	   dstImage = (GLushort *)malloc(memReq);
	   break;
	 case GL_UNSIGNED_INT_8_8_8_8:
	 case GL_UNSIGNED_INT_8_8_8_8_REV:
	 case GL_UNSIGNED_INT_10_10_10_2:
	 case GL_UNSIGNED_INT_2_10_10_10_REV:
	   dstImage = (GLuint *)malloc(memReq);
	   break;
	 default:
	   return GLU_INVALID_ENUM; /* assertion */
	 }
	 if (dstImage == NULL) {
	   glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
	   glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
	   glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
	   glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
	   glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
	   glPixelStorei(GL_UNPACK_SKIP_IMAGES, psm.unpack_skip_images);
	   glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, psm.unpack_image_height);
	   free(srcImage);
	   return GLU_OUT_OF_MEMORY;
	 }
       }
       /* level userLevel is in srcImage; nothing saved yet */
       level = userLevel;       
   }

   glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
   if (baseLevel <= level && level <= maxLevel) {
     gluTexImage3D(target, level, internalFormat, newWidth, newHeight, newDepth,
		  0,format, type, (void *)srcImage);
   }
   level++; /* update current level for the loop */
   for (; level <= levels; level++) {
       switch(type) {
	   case GL_UNSIGNED_BYTE:
	       if (newDepth > 1) {
	       halveImage3D(cmpts,extractUbyte,shoveUbyte,
			    newWidth,newHeight,newDepth,
			    srcImage,dstImage,elementSize,groupSize,rowSize,
			    imageSize,myswapBytes);
	       }
	       else {
		 halveImage_ubyte(cmpts,newWidth,newHeight,srcImage,dstImage,
				  elementSize,rowSize,groupSize);
	       }
	       break;
	   case GL_BYTE:
	       if (newDepth > 1) {
	       halveImage3D(cmpts,extractSbyte,shoveSbyte,
			    newWidth,newHeight,newDepth,
			    srcImage,dstImage,elementSize,groupSize,rowSize,
			    imageSize,myswapBytes);
	       }
	       else {
		 halveImage_byte(cmpts,newWidth,newHeight,srcImage,dstImage,
				  elementSize,rowSize,groupSize);
	       }
	       break;
	   case GL_UNSIGNED_SHORT:
	       if (newDepth > 1) {
	       halveImage3D(cmpts,extractUshort,shoveUshort,
			    newWidth,newHeight,newDepth,
			    srcImage,dstImage,elementSize,groupSize,rowSize,
			    imageSize,myswapBytes);
	       }
	       else {
		 halveImage_ushort(cmpts,newWidth,newHeight,srcImage,dstImage,
				   elementSize,rowSize,groupSize,myswapBytes);
	       }
	       break;
	   case GL_SHORT:
	       if (newDepth > 1) {
	       halveImage3D(cmpts,extractSshort,shoveSshort,
			    newWidth,newHeight,newDepth,
			    srcImage,dstImage,elementSize,groupSize,rowSize,
			    imageSize,myswapBytes);
	       }
	       else {
		 halveImage_short(cmpts,newWidth,newHeight,srcImage,dstImage,
				  elementSize,rowSize,groupSize,myswapBytes);
	       }
	       break;
	   case GL_UNSIGNED_INT:
	       if (newDepth > 1) {
	       halveImage3D(cmpts,extractUint,shoveUint,
			    newWidth,newHeight,newDepth,
			    srcImage,dstImage,elementSize,groupSize,rowSize,
			    imageSize,myswapBytes);
	       }
	       else {
		 halveImage_uint(cmpts,newWidth,newHeight,srcImage,dstImage,
				 elementSize,rowSize,groupSize,myswapBytes);
	       }
	       break;
	   case GL_INT:
	       if (newDepth > 1) {
	       halveImage3D(cmpts,extractSint,shoveSint,
			    newWidth,newHeight,newDepth,
			    srcImage,dstImage,elementSize,groupSize,rowSize,
			    imageSize,myswapBytes);
	       }
	       else {
		 halveImage_int(cmpts,newWidth,newHeight,srcImage,dstImage,
				elementSize,rowSize,groupSize,myswapBytes);
	       }
	       break;
	   case GL_FLOAT:
	       if (newDepth > 1) {
	       halveImage3D(cmpts,extractFloat,shoveFloat,
			    newWidth,newHeight,newDepth,
			    srcImage,dstImage,elementSize,groupSize,rowSize,
			    imageSize,myswapBytes);
	       }
	       else {
		 halveImage_float(cmpts,newWidth,newHeight,srcImage,dstImage,
				  elementSize,rowSize,groupSize,myswapBytes);
	       }
	       break;
	   case GL_UNSIGNED_BYTE_3_3_2:
	       halveImagePackedPixel3D(3,extract332,shove332,
				       newWidth,newHeight,newDepth,
				       srcImage,dstImage,elementSize,rowSize,
				       imageSize,myswapBytes);
	       break;
	   case GL_UNSIGNED_BYTE_2_3_3_REV:
	       halveImagePackedPixel3D(3,extract233rev,shove233rev,
				       newWidth,newHeight,newDepth,
				       srcImage,dstImage,elementSize,rowSize,
				       imageSize,myswapBytes);
	       break;
	   case GL_UNSIGNED_SHORT_5_6_5:
	       halveImagePackedPixel3D(3,extract565,shove565,
				       newWidth,newHeight,newDepth,
				       srcImage,dstImage,elementSize,rowSize,
				       imageSize,myswapBytes);
	       break;
	   case GL_UNSIGNED_SHORT_5_6_5_REV:
	       halveImagePackedPixel3D(3,extract565rev,shove565rev,
				       newWidth,newHeight,newDepth,
				       srcImage,dstImage,elementSize,rowSize,
				       imageSize,myswapBytes);
	       break;
	   case GL_UNSIGNED_SHORT_4_4_4_4:
	       halveImagePackedPixel3D(4,extract4444,shove4444,
				       newWidth,newHeight,newDepth,
				       srcImage,dstImage,elementSize,rowSize,
				       imageSize,myswapBytes);
	       break;
	   case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	       halveImagePackedPixel3D(4,extract4444rev,shove4444rev,
				       newWidth,newHeight,newDepth,
				       srcImage,dstImage,elementSize,rowSize,
				       imageSize,myswapBytes);
	       break;
	   case GL_UNSIGNED_SHORT_5_5_5_1:	        
	       halveImagePackedPixel3D(4,extract5551,shove5551,
				       newWidth,newHeight,newDepth,
				       srcImage,dstImage,elementSize,rowSize,
				       imageSize,myswapBytes);
	       break;
	   case GL_UNSIGNED_SHORT_1_5_5_5_REV:	        
	       halveImagePackedPixel3D(4,extract1555rev,shove1555rev,
				       newWidth,newHeight,newDepth,
				       srcImage,dstImage,elementSize,rowSize,
				       imageSize,myswapBytes);
	       break;
	   case GL_UNSIGNED_INT_8_8_8_8:
	       halveImagePackedPixel3D(4,extract8888,shove8888,
				       newWidth,newHeight,newDepth,
				       srcImage,dstImage,elementSize,rowSize,
				       imageSize,myswapBytes);
	       break;
	   case GL_UNSIGNED_INT_8_8_8_8_REV:
	       halveImagePackedPixel3D(4,extract8888rev,shove8888rev,
				       newWidth,newHeight,newDepth,
				       srcImage,dstImage,elementSize,rowSize,
				       imageSize,myswapBytes);
	       break;
	   case GL_UNSIGNED_INT_10_10_10_2:
	       halveImagePackedPixel3D(4,extract1010102,shove1010102,
				       newWidth,newHeight,newDepth,
				       srcImage,dstImage,elementSize,rowSize,
				       imageSize,myswapBytes);
	       break;
	   case GL_UNSIGNED_INT_2_10_10_10_REV:
	       halveImagePackedPixel3D(4,extract2101010rev,shove2101010rev,
				       newWidth,newHeight,newDepth,
				       srcImage,dstImage,elementSize,rowSize,
				       imageSize,myswapBytes);
	       break;
	   default:
	       assert(0);
	       break;
       }

       __GLU_SWAP_IMAGE(srcImage,dstImage);

       if (newWidth > 1) { newWidth /= 2; rowSize /= 2;}
       if (newHeight > 1) { newHeight /= 2; imageSize = rowSize * newHeight; }
       if (newDepth > 1) newDepth /= 2;
       {
	  /* call tex image with srcImage untouched since it's not padded */
	  if (baseLevel <= level && level <= maxLevel) {
	    gluTexImage3D(target, level, internalFormat, newWidth, newHeight,
			 newDepth,0, format, type, (void *) srcImage);
	  }
       }
   } /* for level */
   glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
   glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
   glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
   glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
   glPixelStorei(GL_UNPACK_SKIP_IMAGES, psm.unpack_skip_images);
   glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, psm.unpack_image_height);

   free(srcImage); /*if you get to here, a srcImage has always been malloc'ed*/
   if (dstImage) { /* if it's non-rectangular and only 1 level */
     free(dstImage);
   }
   return 0;
} /* gluBuild3DMipmapLevelsCore() */

GLint GLAPIENTRY
gluBuild3DMipmapLevels(GLenum target, GLint internalFormat,
			     GLsizei width, GLsizei height, GLsizei depth,
			     GLenum format, GLenum type,
			     GLint userLevel, GLint baseLevel, GLint maxLevel,
			     const void *data)
{
   int level, levels;

   int rc= checkMipmapArgs(internalFormat,format,type);
   if (rc != 0) return rc;

   if (width < 1 || height < 1 || depth < 1) {
       return GLU_INVALID_VALUE;
   }

   if(type == GL_BITMAP) {
      return GLU_INVALID_ENUM;
   }

   levels = computeLog(width);
   level = computeLog(height);
   if (level > levels) levels=level;
   level = computeLog(depth);
   if (level > levels) levels=level;

   levels+= userLevel;
   if (!isLegalLevels(userLevel,baseLevel,maxLevel,levels))
      return GLU_INVALID_VALUE;

   return gluBuild3DMipmapLevelsCore(target, internalFormat,
				     width, height, depth,
				     width, height, depth,
				     format, type,
				     userLevel, baseLevel, maxLevel,
				     data);
} /* gluBuild3DMipmapLevels() */

GLint GLAPIENTRY
gluBuild3DMipmaps(GLenum target, GLint internalFormat,
			GLsizei width, GLsizei height, GLsizei depth,
			GLenum format, GLenum type, const void *data)
{
   GLint widthPowerOf2, heightPowerOf2, depthPowerOf2;
   int level, levels;

   int rc= checkMipmapArgs(internalFormat,format,type);
   if (rc != 0) return rc;

   if (width < 1 || height < 1 || depth < 1) {
       return GLU_INVALID_VALUE;
   }

   if(type == GL_BITMAP) {
      return GLU_INVALID_ENUM;
   }

   closestFit3D(target,width,height,depth,internalFormat,format,type,
		&widthPowerOf2,&heightPowerOf2,&depthPowerOf2);

   levels = computeLog(widthPowerOf2);
   level = computeLog(heightPowerOf2);
   if (level > levels) levels=level;
   level = computeLog(depthPowerOf2);
   if (level > levels) levels=level;

   return gluBuild3DMipmapLevelsCore(target, internalFormat,
				     width, height, depth,
				     widthPowerOf2, heightPowerOf2,
				     depthPowerOf2,
				     format, type, 0, 0, levels,
				     data);
} /* gluBuild3DMipmaps() */

static GLdouble extractUbyte(int isSwap, const void *ubyte)
{
   isSwap= isSwap;		/* turn off warnings */

   assert(*((const GLubyte *)ubyte) <= 255);

   return (GLdouble)(*((const GLubyte *)ubyte));
} /* extractUbyte() */

static void shoveUbyte(GLdouble value, int index, void *data)
{
   assert(0.0 <= value && value < 256.0);

   ((GLubyte *)data)[index]= (GLubyte)value;
} /* shoveUbyte() */

static GLdouble extractSbyte(int isSwap, const void *sbyte)
{
   isSwap= isSwap;		/* turn off warnings */

   assert(*((const GLbyte *)sbyte) <= 127);

   return (GLdouble)(*((const GLbyte *)sbyte));
} /* extractSbyte() */

static void shoveSbyte(GLdouble value, int index, void *data)
{
   ((GLbyte *)data)[index]= (GLbyte)value;
} /* shoveSbyte() */

static GLdouble extractUshort(int isSwap, const void *uitem)
{
   GLushort ushort;

   if (isSwap) {
     ushort= __GLU_SWAP_2_BYTES(uitem);
   }
   else {
     ushort= *(const GLushort *)uitem;
   }

   assert(ushort <= 65535);

   return (GLdouble)ushort;
} /* extractUshort() */

static void shoveUshort(GLdouble value, int index, void *data)
{
   assert(0.0 <= value && value < 65536.0);

   ((GLushort *)data)[index]= (GLushort)value;
} /* shoveUshort() */

static GLdouble extractSshort(int isSwap, const void *sitem)
{
   GLshort sshort;

   if (isSwap) {
     sshort= __GLU_SWAP_2_BYTES(sitem);
   }
   else {
     sshort= *(const GLshort *)sitem;
   }

   assert(sshort <= 32767);

   return (GLdouble)sshort;
} /* extractSshort() */

static void shoveSshort(GLdouble value, int index, void *data)
{
   assert(0.0 <= value && value < 32768.0);

   ((GLshort *)data)[index]= (GLshort)value;
} /* shoveSshort() */

static GLdouble extractUint(int isSwap, const void *uitem)
{
   GLuint uint;

   if (isSwap) {
     uint= __GLU_SWAP_4_BYTES(uitem);
   }
   else {
     uint= *(const GLuint *)uitem;
   }

   assert(uint <= 0xffffffff);

   return (GLdouble)uint;
} /* extractUint() */

static void shoveUint(GLdouble value, int index, void *data)
{
   assert(0.0 <= value && value <= (GLdouble) UINT_MAX);

   ((GLuint *)data)[index]= (GLuint)value;
} /* shoveUint() */

static GLdouble extractSint(int isSwap, const void *sitem)
{
   GLint sint;

   if (isSwap) {
     sint= __GLU_SWAP_4_BYTES(sitem);
   }
   else {
     sint= *(const GLint *)sitem;
   }

   assert(sint <= 0x7fffffff);

   return (GLdouble)sint;
} /* extractSint() */

static void shoveSint(GLdouble value, int index, void *data)
{
   assert(0.0 <= value && value <= (GLdouble) INT_MAX);

   ((GLint *)data)[index]= (GLint)value;
} /* shoveSint() */

static GLdouble extractFloat(int isSwap, const void *item)
{
   GLfloat ffloat;

   if (isSwap) {
     ffloat= __GLU_SWAP_4_BYTES(item);
   }
   else {
     ffloat= *(const GLfloat *)item;
   }

   assert(ffloat <= 1.0);

   return (GLdouble)ffloat;
} /* extractFloat() */

static void shoveFloat(GLdouble value, int index, void *data)
{
   assert(0.0 <= value && value <= 1.0);

   ((GLfloat *)data)[index]= value;
} /* shoveFloat() */

static void halveImageSlice(int components,
			    GLdouble (*extract)(int, const void *),
			    void (*shove)(GLdouble, int, void *),
			    GLint width, GLint height, GLint depth,
			    const void *dataIn, void *dataOut,
			    GLint elementSizeInBytes,
			    GLint groupSizeInBytes,
			    GLint rowSizeInBytes,
			    GLint imageSizeInBytes,
			    GLint isSwap)
{
   int ii, jj;
   int halfWidth= width / 2;
   int halfHeight= height / 2;
   int halfDepth= depth / 2;
   const char *src= (const char *)dataIn;
   int rowPadBytes= rowSizeInBytes - (width * groupSizeInBytes);
   int imagePadBytes= imageSizeInBytes - (width*height*groupSizeInBytes);
   int outIndex= 0;

   assert((width == 1 || height == 1) && depth >= 2);

   if (width == height) {	/* a 1-pixel column viewed from top */
      /* printf("1-column\n");*/
      assert(width == 1 && height == 1);
      assert(depth >= 2);

      for (ii= 0; ii< halfDepth; ii++) {
	 int cc;

	 for (cc = 0; cc < components; cc++) {
	    double totals[4];
	    double extractTotals[BOX2][4];
	    int kk;

	    extractTotals[0][cc]= (*extract)(isSwap,src);
	    extractTotals[1][cc]= (*extract)(isSwap,(src+imageSizeInBytes));

	    /* average 2 pixels since only a column */
	    totals[cc]= 0.0;
	    /* totals[RED]= extractTotals[0][RED]+extractTotals[1][RED];
	     * totals[RED]/= 2.0;
	     */
	    for (kk = 0; kk < BOX2; kk++) {
	      totals[cc]+= extractTotals[kk][cc];
	    }
	    totals[cc]/= (double)BOX2;

	    (*shove)(totals[cc],outIndex,dataOut);
	    outIndex++;
	    src+= elementSizeInBytes;
	 } /* for cc */

	 /* skip over to next group of 2 */
	 src+= rowSizeInBytes;
      } /* for ii */

      assert(src == &((const char *)dataIn)[rowSizeInBytes*height*depth]);
      assert(outIndex == halfDepth * components);
   }
   else if (height == 1) {	/* horizontal slice viewed from top */
      /* printf("horizontal slice\n"); */
      assert(width != 1);

      for (ii= 0; ii< halfDepth; ii++) {
	 for (jj= 0; jj< halfWidth; jj++) {
	    int cc;

	    for (cc = 0; cc < components; cc++) {
	       int kk;
	       double totals[4];
	       double extractTotals[BOX4][4];

	       extractTotals[0][cc]=(*extract)(isSwap,src);
	       extractTotals[1][cc]=(*extract)(isSwap,
					       (src+groupSizeInBytes));
	       extractTotals[2][cc]=(*extract)(isSwap,
					       (src+imageSizeInBytes));
	       extractTotals[3][cc]=(*extract)(isSwap,
					       (src+imageSizeInBytes+groupSizeInBytes));

	       /* grab 4 pixels to average */
	       totals[cc]= 0.0;
	       /* totals[RED]= extractTotals[0][RED]+extractTotals[1][RED]+
		*	       extractTotals[2][RED]+extractTotals[3][RED];
		* totals[RED]/= 4.0;
		*/
	       for (kk = 0; kk < BOX4; kk++) {
		  totals[cc]+= extractTotals[kk][cc];
	       }
	       totals[cc]/= (double)BOX4;

	       (*shove)(totals[cc],outIndex,dataOut);
	       outIndex++;

	       src+= elementSizeInBytes;
	    } /* for cc */

	    /* skip over to next horizontal square of 4 */
	    src+= groupSizeInBytes;
	 } /* for jj */
	 src+= rowPadBytes;

	 src+= rowSizeInBytes;
      } /* for ii */

      assert(src == &((const char *)dataIn)[rowSizeInBytes*height*depth]);
      assert(outIndex == halfWidth * halfDepth * components);
   }
   else if (width == 1) {	/* vertical slice viewed from top */
      /* printf("vertical slice\n"); */
      assert(height != 1);

      for (ii= 0; ii< halfDepth; ii++) {
	 for (jj= 0; jj< halfHeight; jj++) {
	    int cc;

	    for (cc = 0; cc < components; cc++) {
	       int kk;
	       double totals[4];
	       double extractTotals[BOX4][4];

	       extractTotals[0][cc]=(*extract)(isSwap,src);
	       extractTotals[1][cc]=(*extract)(isSwap,
					       (src+rowSizeInBytes));
	       extractTotals[2][cc]=(*extract)(isSwap,
					       (src+imageSizeInBytes));
	       extractTotals[3][cc]=(*extract)(isSwap,
					       (src+imageSizeInBytes+rowSizeInBytes));

	       /* grab 4 pixels to average */
	       totals[cc]= 0.0;
	       /* totals[RED]= extractTotals[0][RED]+extractTotals[1][RED]+
		*	       extractTotals[2][RED]+extractTotals[3][RED];
		* totals[RED]/= 4.0;
		*/
	       for (kk = 0; kk < BOX4; kk++) {
		  totals[cc]+= extractTotals[kk][cc];
	       }
	       totals[cc]/= (double)BOX4;

	       (*shove)(totals[cc],outIndex,dataOut);
	       outIndex++;

	       src+= elementSizeInBytes;
	    } /* for cc */
	    src+= rowPadBytes;

	    /* skip over to next vertical square of 4 */
	    src+= rowSizeInBytes;
	 } /* for jj */
         src+= imagePadBytes;

	 src+= imageSizeInBytes;
      } /* for ii */

      assert(src == &((const char *)dataIn)[rowSizeInBytes*height*depth]);
      assert(outIndex == halfHeight * halfDepth * components);
   }

} /* halveImageSlice() */

static void halveImage3D(int components,
			 GLdouble (*extract)(int, const void *),
			 void (*shove)(GLdouble, int, void *),
			 GLint width, GLint height, GLint depth,
			 const void *dataIn, void *dataOut,
			 GLint elementSizeInBytes,
			 GLint groupSizeInBytes,
			 GLint rowSizeInBytes,
			 GLint imageSizeInBytes,
			 GLint isSwap)
{
   assert(depth > 1);

   /* a horizontal/vertical/one-column slice viewed from top */
   if (width == 1 || height == 1) {
      assert(1 <= depth);

      halveImageSlice(components,extract,shove, width, height, depth,
		      dataIn, dataOut, elementSizeInBytes, groupSizeInBytes,
		      rowSizeInBytes, imageSizeInBytes, isSwap);
      return;
   }
   {
      int ii, jj, dd;

      int halfWidth= width / 2;
      int halfHeight= height / 2;
      int halfDepth= depth / 2;
      const char *src= (const char *) dataIn;
      int rowPadBytes= rowSizeInBytes - (width*groupSizeInBytes);
      int imagePadBytes= imageSizeInBytes - (width*height*groupSizeInBytes);
      int outIndex= 0;

      for (dd= 0; dd < halfDepth; dd++) {
	 for (ii= 0; ii< halfHeight; ii++) {
	    for (jj= 0; jj< halfWidth; jj++) {
	       int cc;

	       for (cc= 0; cc < components; cc++) {
		  int kk;
#define BOX8 8
		  double totals[4];	/* 4 is maximum components */
		  double extractTotals[BOX8][4]; /* 4 is maximum components */

		  extractTotals[0][cc]= (*extract)(isSwap,src);
		  extractTotals[1][cc]= (*extract)(isSwap,
						   (src+groupSizeInBytes));
		  extractTotals[2][cc]= (*extract)(isSwap,
						   (src+rowSizeInBytes));
		  extractTotals[3][cc]= (*extract)(isSwap,
						   (src+rowSizeInBytes+groupSizeInBytes));

		  extractTotals[4][cc]= (*extract)(isSwap,
						   (src+imageSizeInBytes));

		  extractTotals[5][cc]= (*extract)(isSwap,
						   (src+groupSizeInBytes+imageSizeInBytes));
		  extractTotals[6][cc]= (*extract)(isSwap,
						   (src+rowSizeInBytes+imageSizeInBytes));
		  extractTotals[7][cc]= (*extract)(isSwap,
						   (src+rowSizeInBytes+groupSizeInBytes+imageSizeInBytes));

		  totals[cc]= 0.0;

		  /* totals[RED]= extractTotals[0][RED]+extractTotals[1][RED]+
		   *		  extractTotals[2][RED]+extractTotals[3][RED]+
		   *		  extractTotals[4][RED]+extractTotals[5][RED]+
		   *		  extractTotals[6][RED]+extractTotals[7][RED];
		   * totals[RED]/= 8.0;
		   */
		  for (kk = 0; kk < BOX8; kk++) {
		     totals[cc]+= extractTotals[kk][cc];
		  }
		  totals[cc]/= (double)BOX8;

		  (*shove)(totals[cc],outIndex,dataOut);

		  outIndex++;

		  src+= elementSizeInBytes; /* go to next component */
	       } /* for cc */

	       /* skip over to next square of 4 */
	       src+= groupSizeInBytes;
	    } /* for jj */
	    /* skip past pad bytes, if any, to get to next row */
	    src+= rowPadBytes;

	    /* src is at beginning of a row here, but it's the second row of
	     * the square block of 4 pixels that we just worked on so we
	     * need to go one more row.
	     * i.e.,
	     *			 OO...
	     *		 here -->OO...
	     *	     but want -->OO...
	     *			 OO...
	     *			 ...
	     */
	    src+= rowSizeInBytes;
	 } /* for ii */

	 /* skip past pad bytes, if any, to get to next image */
	 src+= imagePadBytes;

	 src+= imageSizeInBytes;
      } /* for dd */

      /* both pointers must reach one byte after the end */
      assert(src == &((const char *)dataIn)[rowSizeInBytes*height*depth]);
      assert(outIndex == halfWidth * halfHeight * halfDepth * components);
   }
} /* halveImage3D() */



/*** mipmap.c ***/

