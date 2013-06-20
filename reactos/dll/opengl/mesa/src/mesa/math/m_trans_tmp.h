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
 * \brief  Templates for vector conversions.
 * \author Keith Whitwell.
 */

#ifdef DEST_4F
static void DEST_4F( GLfloat (*t)[4],
		     CONST void *ptr,
		     GLuint stride,
		     ARGS )
{
   const GLubyte *f = (GLubyte *) ptr + SRC_START * stride;
   const GLubyte *first = f;
   GLuint i;

   (void) first;
   (void) start;
   for (i = DST_START ; i < n ; i++, NEXT_F) {
      CHECK {
         NEXT_F2;
	 if (SZ >= 1) t[i][0] = TRX_4F(f, 0);
	 if (SZ >= 2) t[i][1] = TRX_4F(f, 1);
	 if (SZ >= 3) t[i][2] = TRX_4F(f, 2);
	 if (SZ == 4) t[i][3] = TRX_4F(f, 3); else t[i][3] = 1.0;
      }
   }
}
#endif



#ifdef DEST_4FN
static void DEST_4FN( GLfloat (*t)[4],
		      CONST void *ptr,
		      GLuint stride,
		      ARGS )
{
   const GLubyte *f = (GLubyte *) ptr + SRC_START * stride;
   const GLubyte *first = f;
   GLuint i;

   (void) first;
   (void) start;
   for (i = DST_START ; i < n ; i++, NEXT_F) {
      CHECK {
         NEXT_F2;
	 if (SZ >= 1) t[i][0] = TRX_4FN(f, 0);
	 if (SZ >= 2) t[i][1] = TRX_4FN(f, 1);
	 if (SZ >= 3) t[i][2] = TRX_4FN(f, 2);
	 if (SZ == 4) t[i][3] = TRX_4FN(f, 3); else t[i][3] = 1.0;
      }
   }
}
#endif


#ifdef DEST_3FN
static void DEST_3FN( GLfloat (*t)[3],
		     CONST void *ptr,
		     GLuint stride,
		     ARGS )
{
   const GLubyte *f = (GLubyte *) ptr + SRC_START * stride;
   const GLubyte *first = f;
   GLuint i;
   (void) first;
   (void) start;
   for (i = DST_START ; i < n ; i++, NEXT_F) {
      CHECK {
         NEXT_F2;
	 t[i][0] = TRX_3FN(f, 0);
	 t[i][1] = TRX_3FN(f, 1);
	 t[i][2] = TRX_3FN(f, 2);
      }
   }
}
#endif

#ifdef DEST_1F
static void DEST_1F( GLfloat *t,
		     CONST void *ptr,
		     GLuint stride,
		     ARGS )
{
   const GLubyte *f = (GLubyte *) ptr + SRC_START * stride;
   const GLubyte *first = f;
   GLuint i;
   (void) first;
   (void) start;
   for (i = DST_START ; i < n ; i++, NEXT_F) {
      CHECK {
         NEXT_F2;
	 t[i] = TRX_1F(f, 0);
      }
   }
}
#endif

#ifdef DEST_4UB
static void DEST_4UB( GLubyte (*t)[4],
                      CONST void *ptr,
                      GLuint stride,
                      ARGS )
{
   const GLubyte *f = (GLubyte *) ptr + SRC_START * stride;
   const GLubyte *first = f;
   GLuint i;
   (void) start;
   (void) first;
   for (i = DST_START ; i < n ; i++, NEXT_F) {
      CHECK {
         NEXT_F2;
	 if (SZ >= 1) TRX_UB(t[i][0], f, 0);
	 if (SZ >= 2) TRX_UB(t[i][1], f, 1);
	 if (SZ >= 3) TRX_UB(t[i][2], f, 2);
	 if (SZ == 4) TRX_UB(t[i][3], f, 3); else t[i][3] = 255;
      }
   }
}
#endif


#ifdef DEST_4US
static void DEST_4US( GLushort (*t)[4],
                      CONST void *ptr,
                      GLuint stride,
                      ARGS )
{
   const GLubyte *f = (GLubyte *) ((GLubyte *) ptr + SRC_START * stride);
   const GLubyte *first = f;
   GLuint i;
   (void) start;
   (void) first;
   for (i = DST_START ; i < n ; i++, NEXT_F) {
      CHECK {
         NEXT_F2;
	 if (SZ >= 1) TRX_US(t[i][0], f, 0);
	 if (SZ >= 2) TRX_US(t[i][1], f, 1);
	 if (SZ >= 3) TRX_US(t[i][2], f, 2);
	 if (SZ == 4) TRX_US(t[i][3], f, 3); else t[i][3] = 65535;
      }
   }
}
#endif


#ifdef DEST_1UB
static void DEST_1UB( GLubyte *t,
		      CONST void *ptr,
		      GLuint stride,
		      ARGS )
{
   const GLubyte *f = (GLubyte *) ptr + SRC_START * stride;
   const GLubyte *first = f;
   GLuint i;
   (void) start;
   (void) first;
   for (i = DST_START ; i < n ; i++, NEXT_F) {
      CHECK {
         NEXT_F2;
	  TRX_UB(t[i], f, 0);
      }
   }
}
#endif


#ifdef DEST_1UI
static void DEST_1UI( GLuint *t,
		      CONST void *ptr,
		      GLuint stride,
		      ARGS )
{
   const GLubyte *f = (GLubyte *) ptr + SRC_START * stride;
   const GLubyte *first = f;
   GLuint i;
   (void) start;
   (void) first;

   for (i = DST_START ; i < n ; i++, NEXT_F) {
      CHECK {
         NEXT_F2;
	 t[i] = TRX_UI(f, 0);
      }
   }
}
#endif


static void INIT(void)
{
#ifdef DEST_1UI
   ASSERT(SZ == 1);
   TAB(_1ui)[SRC_IDX] = DEST_1UI;
#endif
#ifdef DEST_1UB
   ASSERT(SZ == 1);
   TAB(_1ub)[SRC_IDX] = DEST_1UB;
#endif
#ifdef DEST_1F
   ASSERT(SZ == 1);
   TAB(_1f)[SRC_IDX] = DEST_1F;
#endif
#ifdef DEST_3FN
   ASSERT(SZ == 3);
   TAB(_3fn)[SRC_IDX] = DEST_3FN;
#endif
#ifdef DEST_4UB
   TAB(_4ub)[SZ][SRC_IDX] = DEST_4UB;
#endif
#ifdef DEST_4US
   TAB(_4us)[SZ][SRC_IDX] = DEST_4US;
#endif
#ifdef DEST_4F
   TAB(_4f)[SZ][SRC_IDX] = DEST_4F;
#endif
#ifdef DEST_4FN
   TAB(_4fn)[SZ][SRC_IDX] = DEST_4FN;
#endif

}


#ifdef INIT
#undef INIT
#endif
#ifdef DEST_1UI
#undef DEST_1UI
#endif
#ifdef DEST_1UB
#undef DEST_1UB
#endif
#ifdef DEST_4UB
#undef DEST_4UB
#endif
#ifdef DEST_4US
#undef DEST_4US
#endif
#ifdef DEST_3FN
#undef DEST_3FN
#endif
#ifdef DEST_4F
#undef DEST_4F
#endif
#ifdef DEST_4FN
#undef DEST_4FN
#endif
#ifdef DEST_1F
#undef DEST_1F
#endif
#ifdef SZ
#undef SZ
#endif
#ifdef TAG
#undef TAG
#endif

