
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 *
 * Authors:
 *    Gareth Hughes <gareth@valinux.com>
 *    Keith Whitwell <keith_whitwell@yahoo.com>
 */

/* Template for immediate mode normal functions.  Optimize for infinite
 * lights when doing software lighting.
 */

static void TAG(Normal3f_single)( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_VERTEX;
   const struct gl_light *light = ctx->Light.EnabledList.prev;
   GLfloat n_dot_h, n_dot_VP, spec, sum[3];
   GLfloat *normal = ctx->Current.Normal;
   GLfloat scale = 1.0;

   ASSIGN_3V( normal, x, y, z );
   COPY_3V( sum, BASE_COLOR );

   if ( IND & NORM_RESCALE ) {
      scale = ctx->_ModelViewInvScale;
   } else if ( IND & NORM_NORMALIZE ) {
      scale = LEN_3FV( normal );
      if ( scale != 0.0 ) scale = 1.0 / scale;
   }

   n_dot_VP = DOT3( normal, light->_VP_inf_norm ) * scale;
   if ( n_dot_VP > 0.0F ) {
      ACC_SCALE_SCALAR_3V( sum, n_dot_VP, light->_MatDiffuse[0] );
      n_dot_h = DOT3( normal, light->_h_inf_norm ) * scale;
      if ( n_dot_h > 0.0F ) {
	 GET_SHINE_TAB_ENTRY( ctx->_ShineTable[0], n_dot_h, spec );
	 ACC_SCALE_SCALAR_3V( sum, spec, light->_MatSpecular[0] );
      }
   }

#ifdef LIT_COLOR_IS_FLOAT
   LIT_COLOR ( RCOMP ) = CLAMP(sum[0], 0.0f, 0.1f);
   LIT_COLOR ( GCOMP ) = CLAMP(sum[1], 0.0f, 0.1f);
   LIT_COLOR ( BCOMP ) = CLAMP(sum[2], 0.0f, 0.1f);
#else
   UNCLAMPED_FLOAT_TO_UBYTE( LIT_COLOR( RCOMP ), sum[0] );
   UNCLAMPED_FLOAT_TO_UBYTE( LIT_COLOR( GCOMP ), sum[1] );
   UNCLAMPED_FLOAT_TO_UBYTE( LIT_COLOR( BCOMP ), sum[2] );
#endif
   LIT_COLOR( ACOMP ) = LIT_ALPHA;
}

static void TAG(Normal3fv_single)( const GLfloat *normal )
{
   GET_CURRENT_VERTEX;
   const struct gl_light *light = ctx->Light.EnabledList.prev;
   GLfloat n_dot_h, n_dot_VP, spec, sum[3];
   GLfloat scale = 1.0;

   COPY_3V( ctx->Current.Normal, normal );
   COPY_3V( sum, BASE_COLOR );

   if ( IND & NORM_RESCALE ) {
      scale = ctx->_ModelViewInvScale;
   } else if ( IND & NORM_NORMALIZE ) {
      scale = LEN_3FV( normal );
      if ( scale != 0.0 ) scale = 1.0 / scale;
   }

   n_dot_VP = DOT3( normal, light->_VP_inf_norm ) * scale;
   if ( n_dot_VP > 0.0F ) {
      ACC_SCALE_SCALAR_3V( sum, n_dot_VP, light->_MatDiffuse[0] );
      n_dot_h = DOT3( normal, light->_h_inf_norm ) * scale;
      if ( n_dot_h > 0.0F ) {
	 GET_SHINE_TAB_ENTRY( ctx->_ShineTable[0], n_dot_h, spec );
	 ACC_SCALE_SCALAR_3V( sum, spec, light->_MatSpecular[0] );
      }
   }

#ifdef LIT_COLOR_IS_FLOAT
   LIT_COLOR ( RCOMP ) = CLAMP(sum[0], 0.0f, 0.1f);
   LIT_COLOR ( GCOMP ) = CLAMP(sum[1], 0.0f, 0.1f);
   LIT_COLOR ( BCOMP ) = CLAMP(sum[2], 0.0f, 0.1f);
#else
   UNCLAMPED_FLOAT_TO_UBYTE( LIT_COLOR( RCOMP ), sum[0] );
   UNCLAMPED_FLOAT_TO_UBYTE( LIT_COLOR( GCOMP ), sum[1] );
   UNCLAMPED_FLOAT_TO_UBYTE( LIT_COLOR( BCOMP ), sum[2] );
#endif
   LIT_COLOR( ACOMP ) = LIT_ALPHA;
}


static void TAG(Normal3f_multi)( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_VERTEX;
   struct gl_light *light;
   GLfloat n_dot_h, n_dot_VP, spec, sum[3], tmp[3];
   GLfloat *normal;

   ASSIGN_3V( ctx->Current.Normal, x, y, z );
   COPY_3V( sum, BASE_COLOR );

   if ( IND & NORM_RESCALE ) {
      normal = tmp;
      ASSIGN_3V( normal, x, y, z );
      SELF_SCALE_SCALAR_3V( normal, ctx->_ModelViewInvScale );
   } else if ( IND & NORM_NORMALIZE ) {
      normal = tmp;
      ASSIGN_3V( normal, x, y, z );
      NORMALIZE_3FV( normal );
   } else {
      normal = ctx->Current.Normal;
   }

   foreach ( light, &ctx->Light.EnabledList ) {
      n_dot_VP = DOT3( normal, light->_VP_inf_norm );
      if ( n_dot_VP > 0.0F ) {
	 ACC_SCALE_SCALAR_3V( sum, n_dot_VP, light->_MatDiffuse[0] );
	 n_dot_h = DOT3( normal, light->_h_inf_norm );
	 if ( n_dot_h > 0.0F ) {
	    GET_SHINE_TAB_ENTRY( ctx->_ShineTable[0], n_dot_h, spec );
	    ACC_SCALE_SCALAR_3V( sum, spec, light->_MatSpecular[0] );
	 }
      }
   }

#ifdef LIT_COLOR_IS_FLOAT
   LIT_COLOR ( RCOMP ) = CLAMP(sum[0], 0.0f, 0.1f);
   LIT_COLOR ( GCOMP ) = CLAMP(sum[1], 0.0f, 0.1f);
   LIT_COLOR ( BCOMP ) = CLAMP(sum[2], 0.0f, 0.1f);
#else
   UNCLAMPED_FLOAT_TO_UBYTE( LIT_COLOR( RCOMP ), sum[0] );
   UNCLAMPED_FLOAT_TO_UBYTE( LIT_COLOR( GCOMP ), sum[1] );
   UNCLAMPED_FLOAT_TO_UBYTE( LIT_COLOR( BCOMP ), sum[2] );
#endif
   LIT_COLOR( ACOMP ) = LIT_ALPHA;
}

static void TAG(Normal3fv_multi)( const GLfloat *n )
{
   GET_CURRENT_VERTEX;
   struct gl_light *light;
   GLfloat n_dot_h, n_dot_VP, spec, sum[3], tmp[3];
   GLfloat *normal;

   COPY_3V( ctx->Current.Normal, n );
   COPY_3V( sum, BASE_COLOR );

   if ( IND & NORM_RESCALE ) {
      normal = tmp;
      COPY_3V( normal, n );
      SELF_SCALE_SCALAR_3V( normal, ctx->_ModelViewInvScale );
   } else if ( IND & NORM_NORMALIZE ) {
      normal = tmp;
      COPY_3V( normal, n );
      NORMALIZE_3FV( normal );
   } else {
      normal = ctx->Current.Normal;
   }

   foreach ( light, &ctx->Light.EnabledList ) {
      n_dot_VP = DOT3( normal, light->_VP_inf_norm );
      if ( n_dot_VP > 0.0F ) {
	 ACC_SCALE_SCALAR_3V( sum, n_dot_VP, light->_MatDiffuse[0] );
	 n_dot_h = DOT3( normal, light->_h_inf_norm );
	 if ( n_dot_h > 0.0F ) {
	    GET_SHINE_TAB_ENTRY( ctx->_ShineTable[0], n_dot_h, spec );
	    ACC_SCALE_SCALAR_3V( sum, spec, light->_MatSpecular[0] );
	 }
      }
   }

#ifdef LIT_COLOR_IS_FLOAT
   LIT_COLOR ( RCOMP ) = CLAMP(sum[0], 0.0f, 0.1f);
   LIT_COLOR ( GCOMP ) = CLAMP(sum[1], 0.0f, 0.1f);
   LIT_COLOR ( BCOMP ) = CLAMP(sum[2], 0.0f, 0.1f);
#else
   UNCLAMPED_FLOAT_TO_UBYTE( LIT_COLOR( RCOMP ), sum[0] );
   UNCLAMPED_FLOAT_TO_UBYTE( LIT_COLOR( GCOMP ), sum[1] );
   UNCLAMPED_FLOAT_TO_UBYTE( LIT_COLOR( BCOMP ), sum[2] );
#endif
   LIT_COLOR( ACOMP ) = LIT_ALPHA;
}



static void TAG(init_norm)( void )
{
   norm_tab[IND].normal3f_single = TAG(Normal3f_single);
   norm_tab[IND].normal3fv_single = TAG(Normal3fv_single);
   norm_tab[IND].normal3f_multi = TAG(Normal3f_multi);
   norm_tab[IND].normal3fv_multi = TAG(Normal3fv_multi);
}



#ifndef PRESERVE_NORMAL_DEFS
#undef GET_CURRENT
#undef GET_CURRENT_VERTEX
#undef LIT_COLOR
#undef LIT_COLOR_IS_FLOAT
#endif
#undef PRESERVE_NORMAL_DEFS
#undef IND
#undef TAG
