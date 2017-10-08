/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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

/**
 * \brief  Translate vectors of numbers between various types.
 * \author Keith Whitwell.
 */


#include <precomp.h>

#include "main/mtypes.h"		/* GLchan hack */

typedef void (*trans_1f_func)(GLfloat *to,
			      CONST void *ptr,
			      GLuint stride,
			      GLuint start,
			      GLuint n );

typedef void (*trans_1ui_func)(GLuint *to,
			       CONST void *ptr,
			       GLuint stride,
			       GLuint start,
			       GLuint n );

typedef void (*trans_1ub_func)(GLubyte *to,
			       CONST void *ptr,
			       GLuint stride,
			       GLuint start,
			       GLuint n );

typedef void (*trans_4ub_func)(GLubyte (*to)[4],
                               CONST void *ptr,
                               GLuint stride,
                               GLuint start,
                               GLuint n );

typedef void (*trans_4us_func)(GLushort (*to)[4],
                               CONST void *ptr,
                               GLuint stride,
                               GLuint start,
                               GLuint n );

typedef void (*trans_4f_func)(GLfloat (*to)[4],
			      CONST void *ptr,
			      GLuint stride,
			      GLuint start,
			      GLuint n );

typedef void (*trans_3fn_func)(GLfloat (*to)[3],
			      CONST void *ptr,
			      GLuint stride,
			      GLuint start,
			      GLuint n );




#define TYPE_IDX(t) ((t) & 0xf)
#define MAX_TYPES TYPE_IDX(GL_DOUBLE)+1      /* 0xa + 1 */


/* This macro is used on other systems, so undefine it for this module */

#undef	CHECK

static trans_1f_func  _math_trans_1f_tab[MAX_TYPES];
static trans_1ui_func _math_trans_1ui_tab[MAX_TYPES];
static trans_1ub_func _math_trans_1ub_tab[MAX_TYPES];
static trans_3fn_func  _math_trans_3fn_tab[MAX_TYPES];
static trans_4ub_func _math_trans_4ub_tab[5][MAX_TYPES];
static trans_4us_func _math_trans_4us_tab[5][MAX_TYPES];
static trans_4f_func  _math_trans_4f_tab[5][MAX_TYPES];
static trans_4f_func  _math_trans_4fn_tab[5][MAX_TYPES];


#define PTR_ELT(ptr, elt) (((SRC *)ptr)[elt])


#define TAB(x) _math_trans##x##_tab
#define ARGS   GLuint start, GLuint n
#define SRC_START  start
#define DST_START  0
#define STRIDE stride
#define NEXT_F f += stride
#define NEXT_F2
#define CHECK




/**
 * Translate from GL_BYTE.
 */
#define SRC GLbyte
#define SRC_IDX TYPE_IDX(GL_BYTE)
#define TRX_3FN(f,n)   BYTE_TO_FLOAT( PTR_ELT(f,n) )
#if 1
#define TRX_4F(f,n)   BYTE_TO_FLOAT( PTR_ELT(f,n) )
#else
#define TRX_4F(f,n)   (GLfloat)( PTR_ELT(f,n) )
#endif
#define TRX_4FN(f,n)   BYTE_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_UB(ub, f,n)  ub = BYTE_TO_UBYTE( PTR_ELT(f,n) )
#define TRX_US(ch, f,n)  ch = BYTE_TO_USHORT( PTR_ELT(f,n) )
#define TRX_UI(f,n)  (PTR_ELT(f,n) < 0 ? 0 : (GLuint)  PTR_ELT(f,n))


#define SZ 4
#define INIT init_trans_4_GLbyte_raw
#define DEST_4F trans_4_GLbyte_4f_raw
#define DEST_4FN trans_4_GLbyte_4fn_raw
#define DEST_4UB trans_4_GLbyte_4ub_raw
#define DEST_4US trans_4_GLbyte_4us_raw
#include "m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLbyte_raw
#define DEST_4F trans_3_GLbyte_4f_raw
#define DEST_4FN trans_3_GLbyte_4fn_raw
#define DEST_4UB trans_3_GLbyte_4ub_raw
#define DEST_4US trans_3_GLbyte_4us_raw
#define DEST_3FN trans_3_GLbyte_3fn_raw
#include "m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLbyte_raw
#define DEST_4F trans_2_GLbyte_4f_raw
#define DEST_4FN trans_2_GLbyte_4fn_raw
#include "m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLbyte_raw
#define DEST_4F trans_1_GLbyte_4f_raw
#define DEST_4FN trans_1_GLbyte_4fn_raw
#define DEST_1UB trans_1_GLbyte_1ub_raw
#define DEST_1UI trans_1_GLbyte_1ui_raw
#include "m_trans_tmp.h"

#undef SRC
#undef TRX_3FN
#undef TRX_4F
#undef TRX_4FN
#undef TRX_UB
#undef TRX_US
#undef TRX_UI
#undef SRC_IDX


/**
 * Translate from GL_UNSIGNED_BYTE.
 */
#define SRC GLubyte
#define SRC_IDX TYPE_IDX(GL_UNSIGNED_BYTE)
#define TRX_3FN(f,n)	     UBYTE_TO_FLOAT(PTR_ELT(f,n))
#define TRX_4F(f,n)	     (GLfloat)( PTR_ELT(f,n) )
#define TRX_4FN(f,n)	     UBYTE_TO_FLOAT(PTR_ELT(f,n))
#define TRX_UB(ub, f,n)	     ub = PTR_ELT(f,n)
#define TRX_US(us, f,n)      us = UBYTE_TO_USHORT(PTR_ELT(f,n))
#define TRX_UI(f,n)          (GLuint)PTR_ELT(f,n)

/* 4ub->4ub handled in special case below.
 */
#define SZ 4
#define INIT init_trans_4_GLubyte_raw
#define DEST_4F trans_4_GLubyte_4f_raw
#define DEST_4FN trans_4_GLubyte_4fn_raw
#define DEST_4US trans_4_GLubyte_4us_raw
#include "m_trans_tmp.h"


#define SZ 3
#define INIT init_trans_3_GLubyte_raw
#define DEST_4UB trans_3_GLubyte_4ub_raw
#define DEST_4US trans_3_GLubyte_4us_raw
#define DEST_3FN trans_3_GLubyte_3fn_raw
#define DEST_4F trans_3_GLubyte_4f_raw
#define DEST_4FN trans_3_GLubyte_4fn_raw
#include "m_trans_tmp.h"


#define SZ 1
#define INIT init_trans_1_GLubyte_raw
#define DEST_1UI trans_1_GLubyte_1ui_raw
#define DEST_1UB trans_1_GLubyte_1ub_raw
#include "m_trans_tmp.h"

#undef SRC
#undef SRC_IDX
#undef TRX_3FN
#undef TRX_4F
#undef TRX_4FN
#undef TRX_UB
#undef TRX_US
#undef TRX_UI


/* GL_SHORT
 */
#define SRC GLshort
#define SRC_IDX TYPE_IDX(GL_SHORT)
#define TRX_3FN(f,n)   SHORT_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_4F(f,n)   (GLfloat)( PTR_ELT(f,n) )
#define TRX_4FN(f,n)  SHORT_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_UB(ub, f,n)  ub = SHORT_TO_UBYTE(PTR_ELT(f,n))
#define TRX_US(us, f,n)  us = SHORT_TO_USHORT(PTR_ELT(f,n))
#define TRX_UI(f,n)  (PTR_ELT(f,n) < 0 ? 0 : (GLuint)  PTR_ELT(f,n))


#define SZ  4
#define INIT init_trans_4_GLshort_raw
#define DEST_4F trans_4_GLshort_4f_raw
#define DEST_4FN trans_4_GLshort_4fn_raw
#define DEST_4UB trans_4_GLshort_4ub_raw
#define DEST_4US trans_4_GLshort_4us_raw
#include "m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLshort_raw
#define DEST_4F trans_3_GLshort_4f_raw
#define DEST_4FN trans_3_GLshort_4fn_raw
#define DEST_4UB trans_3_GLshort_4ub_raw
#define DEST_4US trans_3_GLshort_4us_raw
#define DEST_3FN trans_3_GLshort_3fn_raw
#include "m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLshort_raw
#define DEST_4F trans_2_GLshort_4f_raw
#define DEST_4FN trans_2_GLshort_4fn_raw
#include "m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLshort_raw
#define DEST_4F trans_1_GLshort_4f_raw
#define DEST_4FN trans_1_GLshort_4fn_raw
#define DEST_1UB trans_1_GLshort_1ub_raw
#define DEST_1UI trans_1_GLshort_1ui_raw
#include "m_trans_tmp.h"


#undef SRC
#undef SRC_IDX
#undef TRX_3FN
#undef TRX_4F
#undef TRX_4FN
#undef TRX_UB
#undef TRX_US
#undef TRX_UI


/* GL_UNSIGNED_SHORT
 */
#define SRC GLushort
#define SRC_IDX TYPE_IDX(GL_UNSIGNED_SHORT)
#define TRX_3FN(f,n)   USHORT_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_4F(f,n)   (GLfloat)( PTR_ELT(f,n) )
#define TRX_4FN(f,n)  USHORT_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_UB(ub,f,n)  ub = (GLubyte) (PTR_ELT(f,n) >> 8)
#define TRX_US(us,f,n)  us = PTR_ELT(f,n)
#define TRX_UI(f,n)  (GLuint)   PTR_ELT(f,n)


#define SZ 4
#define INIT init_trans_4_GLushort_raw
#define DEST_4F trans_4_GLushort_4f_raw
#define DEST_4FN trans_4_GLushort_4fn_raw
#define DEST_4UB trans_4_GLushort_4ub_raw
#define DEST_4US trans_4_GLushort_4us_raw
#include "m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLushort_raw
#define DEST_4F trans_3_GLushort_4f_raw
#define DEST_4FN trans_3_GLushort_4fn_raw
#define DEST_4UB trans_3_GLushort_4ub_raw
#define DEST_4US trans_3_GLushort_4us_raw
#define DEST_3FN trans_3_GLushort_3fn_raw
#include "m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLushort_raw
#define DEST_4F trans_2_GLushort_4f_raw
#define DEST_4FN trans_2_GLushort_4fn_raw
#include "m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLushort_raw
#define DEST_4F trans_1_GLushort_4f_raw
#define DEST_4FN trans_1_GLushort_4fn_raw
#define DEST_1UB trans_1_GLushort_1ub_raw
#define DEST_1UI trans_1_GLushort_1ui_raw
#include "m_trans_tmp.h"

#undef SRC
#undef SRC_IDX
#undef TRX_3FN
#undef TRX_4F
#undef TRX_4FN
#undef TRX_UB
#undef TRX_US
#undef TRX_UI


/* GL_INT
 */
#define SRC GLint
#define SRC_IDX TYPE_IDX(GL_INT)
#define TRX_3FN(f,n)   INT_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_4F(f,n)   (GLfloat)( PTR_ELT(f,n) )
#define TRX_4FN(f,n)  INT_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_UB(ub, f,n)  ub = INT_TO_UBYTE(PTR_ELT(f,n))
#define TRX_US(us, f,n)  us = INT_TO_USHORT(PTR_ELT(f,n))
#define TRX_UI(f,n)  (PTR_ELT(f,n) < 0 ? 0 : (GLuint)  PTR_ELT(f,n))


#define SZ 4
#define INIT init_trans_4_GLint_raw
#define DEST_4F trans_4_GLint_4f_raw
#define DEST_4FN trans_4_GLint_4fn_raw
#define DEST_4UB trans_4_GLint_4ub_raw
#define DEST_4US trans_4_GLint_4us_raw
#include "m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLint_raw
#define DEST_4F trans_3_GLint_4f_raw
#define DEST_4FN trans_3_GLint_4fn_raw
#define DEST_4UB trans_3_GLint_4ub_raw
#define DEST_4US trans_3_GLint_4us_raw
#define DEST_3FN trans_3_GLint_3fn_raw
#include "m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLint_raw
#define DEST_4F trans_2_GLint_4f_raw
#define DEST_4FN trans_2_GLint_4fn_raw
#include "m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLint_raw
#define DEST_4F trans_1_GLint_4f_raw
#define DEST_4FN trans_1_GLint_4fn_raw
#define DEST_1UB trans_1_GLint_1ub_raw
#define DEST_1UI trans_1_GLint_1ui_raw
#include "m_trans_tmp.h"


#undef SRC
#undef SRC_IDX
#undef TRX_3FN
#undef TRX_4F
#undef TRX_4FN
#undef TRX_UB
#undef TRX_US
#undef TRX_UI


/* GL_UNSIGNED_INT
 */
#define SRC GLuint
#define SRC_IDX TYPE_IDX(GL_UNSIGNED_INT)
#define TRX_3FN(f,n)   INT_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_4F(f,n)   (GLfloat)( PTR_ELT(f,n) )
#define TRX_4FN(f,n)  UINT_TO_FLOAT( PTR_ELT(f,n) )
#define TRX_UB(ub, f,n)  ub = (GLubyte) (PTR_ELT(f,n) >> 24)
#define TRX_US(us, f,n)  us = (GLshort) (PTR_ELT(f,n) >> 16)
#define TRX_UI(f,n)		PTR_ELT(f,n)


#define SZ 4
#define INIT init_trans_4_GLuint_raw
#define DEST_4F trans_4_GLuint_4f_raw
#define DEST_4FN trans_4_GLuint_4fn_raw
#define DEST_4UB trans_4_GLuint_4ub_raw
#define DEST_4US trans_4_GLuint_4us_raw
#include "m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLuint_raw
#define DEST_4F trans_3_GLuint_4f_raw
#define DEST_4FN trans_3_GLuint_4fn_raw
#define DEST_4UB trans_3_GLuint_4ub_raw
#define DEST_4US trans_3_GLuint_4us_raw
#define DEST_3FN trans_3_GLuint_3fn_raw
#include "m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLuint_raw
#define DEST_4F trans_2_GLuint_4f_raw
#define DEST_4FN trans_2_GLuint_4fn_raw
#include "m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLuint_raw
#define DEST_4F trans_1_GLuint_4f_raw
#define DEST_4FN trans_1_GLuint_4fn_raw
#define DEST_1UB trans_1_GLuint_1ub_raw
#define DEST_1UI trans_1_GLuint_1ui_raw
#include "m_trans_tmp.h"

#undef SRC
#undef SRC_IDX
#undef TRX_3FN
#undef TRX_4F
#undef TRX_4FN
#undef TRX_UB
#undef TRX_US
#undef TRX_UI


/* GL_DOUBLE
 */
#define SRC GLdouble
#define SRC_IDX TYPE_IDX(GL_DOUBLE)
#define TRX_3FN(f,n)   (GLfloat) PTR_ELT(f,n)
#define TRX_4F(f,n)   (GLfloat) PTR_ELT(f,n)
#define TRX_4FN(f,n)   (GLfloat) PTR_ELT(f,n)
#define TRX_UB(ub,f,n) UNCLAMPED_FLOAT_TO_UBYTE(ub, PTR_ELT(f,n))
#define TRX_US(us,f,n) UNCLAMPED_FLOAT_TO_USHORT(us, PTR_ELT(f,n))
#define TRX_UI(f,n)  (GLuint) (GLint) PTR_ELT(f,n)
#define TRX_1F(f,n)   (GLfloat) PTR_ELT(f,n)


#define SZ 4
#define INIT init_trans_4_GLdouble_raw
#define DEST_4F trans_4_GLdouble_4f_raw
#define DEST_4FN trans_4_GLdouble_4fn_raw
#define DEST_4UB trans_4_GLdouble_4ub_raw
#define DEST_4US trans_4_GLdouble_4us_raw
#include "m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLdouble_raw
#define DEST_4F trans_3_GLdouble_4f_raw
#define DEST_4FN trans_3_GLdouble_4fn_raw
#define DEST_4UB trans_3_GLdouble_4ub_raw
#define DEST_4US trans_3_GLdouble_4us_raw
#define DEST_3FN trans_3_GLdouble_3fn_raw
#include "m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLdouble_raw
#define DEST_4F trans_2_GLdouble_4f_raw
#define DEST_4FN trans_2_GLdouble_4fn_raw
#include "m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLdouble_raw
#define DEST_4F trans_1_GLdouble_4f_raw
#define DEST_4FN trans_1_GLdouble_4fn_raw
#define DEST_1UB trans_1_GLdouble_1ub_raw
#define DEST_1UI trans_1_GLdouble_1ui_raw
#define DEST_1F trans_1_GLdouble_1f_raw
#include "m_trans_tmp.h"

#undef SRC
#undef SRC_IDX

/* GL_FLOAT
 */
#define SRC GLfloat
#define SRC_IDX TYPE_IDX(GL_FLOAT)
#define SZ 4
#define INIT init_trans_4_GLfloat_raw
#define DEST_4UB trans_4_GLfloat_4ub_raw
#define DEST_4US trans_4_GLfloat_4us_raw
#define DEST_4F  trans_4_GLfloat_4f_raw
#define DEST_4FN  trans_4_GLfloat_4fn_raw
#include "m_trans_tmp.h"

#define SZ 3
#define INIT init_trans_3_GLfloat_raw
#define DEST_4F  trans_3_GLfloat_4f_raw
#define DEST_4FN  trans_3_GLfloat_4fn_raw
#define DEST_4UB trans_3_GLfloat_4ub_raw
#define DEST_4US trans_3_GLfloat_4us_raw
#define DEST_3FN trans_3_GLfloat_3fn_raw
#include "m_trans_tmp.h"

#define SZ 2
#define INIT init_trans_2_GLfloat_raw
#define DEST_4F trans_2_GLfloat_4f_raw
#define DEST_4FN trans_2_GLfloat_4fn_raw
#include "m_trans_tmp.h"

#define SZ 1
#define INIT init_trans_1_GLfloat_raw
#define DEST_4F  trans_1_GLfloat_4f_raw
#define DEST_4FN  trans_1_GLfloat_4fn_raw
#define DEST_1UB trans_1_GLfloat_1ub_raw
#define DEST_1UI trans_1_GLfloat_1ui_raw
#define DEST_1F trans_1_GLfloat_1f_raw

#include "m_trans_tmp.h"

#undef SRC
#undef SRC_IDX
#undef TRX_3FN
#undef TRX_4F
#undef TRX_4FN
#undef TRX_UB
#undef TRX_US
#undef TRX_UI


static void trans_4_GLubyte_4ub_raw(GLubyte (*t)[4],
				    CONST void *Ptr,
				    GLuint stride,
				    ARGS )
{
   const GLubyte *f = (GLubyte *) Ptr + SRC_START * stride;
   GLuint i;

   if (((((uintptr_t) f | (uintptr_t) stride)) & 3L) == 0L) {
      /* Aligned.
       */
      for (i = DST_START ; i < n ; i++, f += stride) {
	 COPY_4UBV( t[i], f );
      }
   } else {
      for (i = DST_START ; i < n ; i++, f += stride) {
	 t[i][0] = f[0];
	 t[i][1] = f[1];
	 t[i][2] = f[2];
	 t[i][3] = f[3];
      }
   }
}


static void init_translate_raw(void)
{
   memset( TAB(_1ui), 0, sizeof(TAB(_1ui)) );
   memset( TAB(_1ub), 0, sizeof(TAB(_1ub)) );
   memset( TAB(_3fn),  0, sizeof(TAB(_3fn)) );
   memset( TAB(_4ub), 0, sizeof(TAB(_4ub)) );
   memset( TAB(_4us), 0, sizeof(TAB(_4us)) );
   memset( TAB(_4f),  0, sizeof(TAB(_4f)) );
   memset( TAB(_4fn),  0, sizeof(TAB(_4fn)) );

   init_trans_4_GLbyte_raw();
   init_trans_3_GLbyte_raw();
   init_trans_2_GLbyte_raw();
   init_trans_1_GLbyte_raw();
   init_trans_1_GLubyte_raw();
   init_trans_3_GLubyte_raw();
   init_trans_4_GLubyte_raw();
   init_trans_4_GLshort_raw();
   init_trans_3_GLshort_raw();
   init_trans_2_GLshort_raw();
   init_trans_1_GLshort_raw();
   init_trans_4_GLushort_raw();
   init_trans_3_GLushort_raw();
   init_trans_2_GLushort_raw();
   init_trans_1_GLushort_raw();
   init_trans_4_GLint_raw();
   init_trans_3_GLint_raw();
   init_trans_2_GLint_raw();
   init_trans_1_GLint_raw();
   init_trans_4_GLuint_raw();
   init_trans_3_GLuint_raw();
   init_trans_2_GLuint_raw();
   init_trans_1_GLuint_raw();
   init_trans_4_GLdouble_raw();
   init_trans_3_GLdouble_raw();
   init_trans_2_GLdouble_raw();
   init_trans_1_GLdouble_raw();
   init_trans_4_GLfloat_raw();
   init_trans_3_GLfloat_raw();
   init_trans_2_GLfloat_raw();
   init_trans_1_GLfloat_raw();

   TAB(_4ub)[4][TYPE_IDX(GL_UNSIGNED_BYTE)] = trans_4_GLubyte_4ub_raw;
}


#undef TAB
#ifdef CLASS
#undef CLASS
#endif
#undef ARGS
#undef CHECK
#undef SRC_START
#undef DST_START
#undef NEXT_F
#undef NEXT_F2





void _math_init_translate( void )
{
   init_translate_raw();
}


/**
 * Translate vector of values to GLfloat [1].
 */
void _math_trans_1f(GLfloat *to,
		    CONST void *ptr,
		    GLuint stride,
		    GLenum type,
		    GLuint start,
		    GLuint n )
{
   _math_trans_1f_tab[TYPE_IDX(type)]( to, ptr, stride, start, n );
}

/**
 * Translate vector of values to GLuint [1].
 */
void _math_trans_1ui(GLuint *to,
		     CONST void *ptr,
		     GLuint stride,
		     GLenum type,
		     GLuint start,
		     GLuint n )
{
   _math_trans_1ui_tab[TYPE_IDX(type)]( to, ptr, stride, start, n );
}

/**
 * Translate vector of values to GLubyte [1].
 */
void _math_trans_1ub(GLubyte *to,
		     CONST void *ptr,
		     GLuint stride,
		     GLenum type,
		     GLuint start,
		     GLuint n )
{
   _math_trans_1ub_tab[TYPE_IDX(type)]( to, ptr, stride, start, n );
}


/**
 * Translate vector of values to GLubyte [4].
 */
void _math_trans_4ub(GLubyte (*to)[4],
		     CONST void *ptr,
		     GLuint stride,
		     GLenum type,
		     GLuint size,
		     GLuint start,
		     GLuint n )
{
   _math_trans_4ub_tab[size][TYPE_IDX(type)]( to, ptr, stride, start, n );
}

/**
 * Translate vector of values to GLchan [4].
 */
void _math_trans_4chan( GLchan (*to)[4],
			CONST void *ptr,
			GLuint stride,
			GLenum type,
			GLuint size,
			GLuint start,
			GLuint n )
{
#if CHAN_TYPE == GL_UNSIGNED_BYTE
   _math_trans_4ub( to, ptr, stride, type, size, start, n );
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
   _math_trans_4us( to, ptr, stride, type, size, start, n );
#elif CHAN_TYPE == GL_FLOAT
   _math_trans_4fn( to, ptr, stride, type, size, start, n );
#endif
}

/**
 * Translate vector of values to GLushort [4].
 */
void _math_trans_4us(GLushort (*to)[4],
		     CONST void *ptr,
		     GLuint stride,
		     GLenum type,
		     GLuint size,
		     GLuint start,
		     GLuint n )
{
   _math_trans_4us_tab[size][TYPE_IDX(type)]( to, ptr, stride, start, n );
}

/**
 * Translate vector of values to GLfloat [4].
 */
void _math_trans_4f(GLfloat (*to)[4],
		    CONST void *ptr,
		    GLuint stride,
		    GLenum type,
		    GLuint size,
		    GLuint start,
		    GLuint n )
{
   _math_trans_4f_tab[size][TYPE_IDX(type)]( to, ptr, stride, start, n );
}

/**
 * Translate vector of values to GLfloat[4], normalized to [-1, 1].
 */
void _math_trans_4fn(GLfloat (*to)[4],
		    CONST void *ptr,
		    GLuint stride,
		    GLenum type,
		    GLuint size,
		    GLuint start,
		    GLuint n )
{
   _math_trans_4fn_tab[size][TYPE_IDX(type)]( to, ptr, stride, start, n );
}

/**
 * Translate vector of values to GLfloat[3], normalized to [-1, 1].
 */
void _math_trans_3fn(GLfloat (*to)[3],
		    CONST void *ptr,
		    GLuint stride,
		    GLenum type,
		    GLuint start,
		    GLuint n )
{
   _math_trans_3fn_tab[TYPE_IDX(type)]( to, ptr, stride, start, n );
}
