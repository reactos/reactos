/**
 * \file mtypes.h
 * Main Mesa data structures.
 *
 * Please try to mark derived values with a leading underscore ('_').
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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



#ifndef TYPES_H
#define TYPES_H


#include "glheader.h"
#include <GL/internal/glcore.h>	/* __GLcontextModes (GLvisual) */
#include "config.h"		/* Hardwired parameters */
#include "glapi/glapitable.h"
#include "glapi/glthread.h"
#include "math/m_matrix.h"	/* GLmatrix */
#include "bitset.h"


/**
 * Special, internal token
 */
#define GL_SHADER_PROGRAM_MESA 0x9999


/**
 * Color channel data type.
 */
#if CHAN_BITS == 8
   typedef GLubyte GLchan;
#define CHAN_MAX 255
#define CHAN_MAXF 255.0F
#define CHAN_TYPE GL_UNSIGNED_BYTE
#elif CHAN_BITS == 16
   typedef GLushort GLchan;
#define CHAN_MAX 65535
#define CHAN_MAXF 65535.0F
#define CHAN_TYPE GL_UNSIGNED_SHORT
#elif CHAN_BITS == 32
   typedef GLfloat GLchan;
#define CHAN_MAX 1.0
#define CHAN_MAXF 1.0F
#define CHAN_TYPE GL_FLOAT
#else
#error "illegal number of color channel bits"
#endif


/**
 * Stencil buffer data type.
 */
#if STENCIL_BITS==8
   typedef GLubyte GLstencil;
#elif STENCIL_BITS==16
   typedef GLushort GLstencil;
#else
#  error "illegal number of stencil bits"
#endif


/**
 * Fixed point data type.
 */
typedef int GLfixed;
/*
 * Fixed point arithmetic macros
 */
#ifndef FIXED_FRAC_BITS
#define FIXED_FRAC_BITS 11
#endif

#define FIXED_SHIFT     FIXED_FRAC_BITS
#define FIXED_ONE       (1 << FIXED_SHIFT)
#define FIXED_HALF      (1 << (FIXED_SHIFT-1))
#define FIXED_FRAC_MASK (FIXED_ONE - 1)
#define FIXED_INT_MASK  (~FIXED_FRAC_MASK)
#define FIXED_EPSILON   1
#define FIXED_SCALE     ((float) FIXED_ONE)
#define FIXED_DBL_SCALE ((double) FIXED_ONE)
#define FloatToFixed(X) (IROUND((X) * FIXED_SCALE))
#define FixedToDouble(X) ((X) * (1.0 / FIXED_DBL_SCALE))
#define IntToFixed(I)   ((I) << FIXED_SHIFT)
#define FixedToInt(X)   ((X) >> FIXED_SHIFT)
#define FixedToUns(X)   (((unsigned int)(X)) >> FIXED_SHIFT)
#define FixedCeil(X)    (((X) + FIXED_ONE - FIXED_EPSILON) & FIXED_INT_MASK)
#define FixedFloor(X)   ((X) & FIXED_INT_MASK)
#define FixedToFloat(X) ((X) * (1.0F / FIXED_SCALE))
#define PosFloatToFixed(X)      FloatToFixed(X)
#define SignedFloatToFixed(X)   FloatToFixed(X)



/**
 * \name Some forward type declarations
 */
/*@{*/
struct _mesa_HashTable;
struct gl_pixelstore_attrib;
struct gl_texture_format;
struct gl_texture_image;
struct gl_texture_object;
typedef struct __GLcontextRec GLcontext;
typedef struct __GLcontextModesRec GLvisual;
typedef struct gl_framebuffer GLframebuffer;
/*@}*/



/**
 * Indexes for vertex program attributes.
 * GL_NV_vertex_program aliases generic attributes over the conventional
 * attributes.  In GL_ARB_vertex_program shader the aliasing is optional.
 * In GL_ARB_vertex_shader / OpenGL 2.0 the aliasing is disallowed (the
 * generic attributes are distinct/separate).
 */
enum
{
   VERT_ATTRIB_POS = 0,
   VERT_ATTRIB_WEIGHT = 1,
   VERT_ATTRIB_NORMAL = 2,
   VERT_ATTRIB_COLOR0 = 3,
   VERT_ATTRIB_COLOR1 = 4,
   VERT_ATTRIB_FOG = 5,
   VERT_ATTRIB_COLOR_INDEX = 6,
   VERT_ATTRIB_EDGEFLAG = 7,
   VERT_ATTRIB_TEX0 = 8,
   VERT_ATTRIB_TEX1 = 9,
   VERT_ATTRIB_TEX2 = 10,
   VERT_ATTRIB_TEX3 = 11,
   VERT_ATTRIB_TEX4 = 12,
   VERT_ATTRIB_TEX5 = 13,
   VERT_ATTRIB_TEX6 = 14,
   VERT_ATTRIB_TEX7 = 15,
   VERT_ATTRIB_GENERIC0 = 16,
   VERT_ATTRIB_GENERIC1 = 17,
   VERT_ATTRIB_GENERIC2 = 18,
   VERT_ATTRIB_GENERIC3 = 19,
   VERT_ATTRIB_GENERIC4 = 20,
   VERT_ATTRIB_GENERIC5 = 21,
   VERT_ATTRIB_GENERIC6 = 22,
   VERT_ATTRIB_GENERIC7 = 23,
   VERT_ATTRIB_GENERIC8 = 24,
   VERT_ATTRIB_GENERIC9 = 25,
   VERT_ATTRIB_GENERIC10 = 26,
   VERT_ATTRIB_GENERIC11 = 27,
   VERT_ATTRIB_GENERIC12 = 28,
   VERT_ATTRIB_GENERIC13 = 29,
   VERT_ATTRIB_GENERIC14 = 30,
   VERT_ATTRIB_GENERIC15 = 31,
   VERT_ATTRIB_MAX = 32
};

/**
 * Bitflags for vertex attributes.
 * These are used in bitfields in many places.
 */
/*@{*/
#define VERT_BIT_POS         (1 << VERT_ATTRIB_POS)
#define VERT_BIT_WEIGHT      (1 << VERT_ATTRIB_WEIGHT)
#define VERT_BIT_NORMAL      (1 << VERT_ATTRIB_NORMAL)
#define VERT_BIT_COLOR0      (1 << VERT_ATTRIB_COLOR0)
#define VERT_BIT_COLOR1      (1 << VERT_ATTRIB_COLOR1)
#define VERT_BIT_FOG         (1 << VERT_ATTRIB_FOG)
#define VERT_BIT_COLOR_INDEX (1 << VERT_ATTRIB_COLOR_INDEX)
#define VERT_BIT_EDGEFLAG    (1 << VERT_ATTRIB_EDGEFLAG)
#define VERT_BIT_TEX0        (1 << VERT_ATTRIB_TEX0)
#define VERT_BIT_TEX1        (1 << VERT_ATTRIB_TEX1)
#define VERT_BIT_TEX2        (1 << VERT_ATTRIB_TEX2)
#define VERT_BIT_TEX3        (1 << VERT_ATTRIB_TEX3)
#define VERT_BIT_TEX4        (1 << VERT_ATTRIB_TEX4)
#define VERT_BIT_TEX5        (1 << VERT_ATTRIB_TEX5)
#define VERT_BIT_TEX6        (1 << VERT_ATTRIB_TEX6)
#define VERT_BIT_TEX7        (1 << VERT_ATTRIB_TEX7)
#define VERT_BIT_GENERIC0    (1 << VERT_ATTRIB_GENERIC0)
#define VERT_BIT_GENERIC1    (1 << VERT_ATTRIB_GENERIC1)
#define VERT_BIT_GENERIC2    (1 << VERT_ATTRIB_GENERIC2)
#define VERT_BIT_GENERIC3    (1 << VERT_ATTRIB_GENERIC3)
#define VERT_BIT_GENERIC4    (1 << VERT_ATTRIB_GENERIC4)
#define VERT_BIT_GENERIC5    (1 << VERT_ATTRIB_GENERIC5)
#define VERT_BIT_GENERIC6    (1 << VERT_ATTRIB_GENERIC6)
#define VERT_BIT_GENERIC7    (1 << VERT_ATTRIB_GENERIC7)
#define VERT_BIT_GENERIC8    (1 << VERT_ATTRIB_GENERIC8)
#define VERT_BIT_GENERIC9    (1 << VERT_ATTRIB_GENERIC9)
#define VERT_BIT_GENERIC10   (1 << VERT_ATTRIB_GENERIC10)
#define VERT_BIT_GENERIC11   (1 << VERT_ATTRIB_GENERIC11)
#define VERT_BIT_GENERIC12   (1 << VERT_ATTRIB_GENERIC12)
#define VERT_BIT_GENERIC13   (1 << VERT_ATTRIB_GENERIC13)
#define VERT_BIT_GENERIC14   (1 << VERT_ATTRIB_GENERIC14)
#define VERT_BIT_GENERIC15   (1 << VERT_ATTRIB_GENERIC15)

#define VERT_BIT_TEX(u)  (1 << (VERT_ATTRIB_TEX0 + (u)))
#define VERT_BIT_GENERIC(g)  (1 << (VERT_ATTRIB_GENERIC0 + (g)))
/*@}*/


/**
 * Indexes for vertex program result attributes
 */
/*@{*/
#define VERT_RESULT_HPOS 0
#define VERT_RESULT_COL0 1
#define VERT_RESULT_COL1 2
#define VERT_RESULT_FOGC 3
#define VERT_RESULT_TEX0 4
#define VERT_RESULT_TEX1 5
#define VERT_RESULT_TEX2 6
#define VERT_RESULT_TEX3 7
#define VERT_RESULT_TEX4 8
#define VERT_RESULT_TEX5 9
#define VERT_RESULT_TEX6 10
#define VERT_RESULT_TEX7 11
#define VERT_RESULT_PSIZ 12
#define VERT_RESULT_BFC0 13
#define VERT_RESULT_BFC1 14
#define VERT_RESULT_EDGE 15
#define VERT_RESULT_VAR0 16  /**< shader varying */
#define VERT_RESULT_MAX  (VERT_RESULT_VAR0 + MAX_VARYING)
/*@}*/


/**
 * Indexes for fragment program input attributes.
 */
enum
{
   FRAG_ATTRIB_WPOS = 0,
   FRAG_ATTRIB_COL0 = 1,
   FRAG_ATTRIB_COL1 = 2,
   FRAG_ATTRIB_FOGC = 3,
   FRAG_ATTRIB_TEX0 = 4,
   FRAG_ATTRIB_TEX1 = 5,
   FRAG_ATTRIB_TEX2 = 6,
   FRAG_ATTRIB_TEX3 = 7,
   FRAG_ATTRIB_TEX4 = 8,
   FRAG_ATTRIB_TEX5 = 9,
   FRAG_ATTRIB_TEX6 = 10,
   FRAG_ATTRIB_TEX7 = 11,
   FRAG_ATTRIB_VAR0 = 12,  /**< shader varying */
   FRAG_ATTRIB_MAX = (FRAG_ATTRIB_VAR0 + MAX_VARYING)
};

/**
 * Bitflags for fragment program input attributes.
 */
/*@{*/
#define FRAG_BIT_WPOS  (1 << FRAG_ATTRIB_WPOS)
#define FRAG_BIT_COL0  (1 << FRAG_ATTRIB_COL0)
#define FRAG_BIT_COL1  (1 << FRAG_ATTRIB_COL1)
#define FRAG_BIT_FOGC  (1 << FRAG_ATTRIB_FOGC)
#define FRAG_BIT_TEX0  (1 << FRAG_ATTRIB_TEX0)
#define FRAG_BIT_TEX1  (1 << FRAG_ATTRIB_TEX1)
#define FRAG_BIT_TEX2  (1 << FRAG_ATTRIB_TEX2)
#define FRAG_BIT_TEX3  (1 << FRAG_ATTRIB_TEX3)
#define FRAG_BIT_TEX4  (1 << FRAG_ATTRIB_TEX4)
#define FRAG_BIT_TEX5  (1 << FRAG_ATTRIB_TEX5)
#define FRAG_BIT_TEX6  (1 << FRAG_ATTRIB_TEX6)
#define FRAG_BIT_TEX7  (1 << FRAG_ATTRIB_TEX7)
#define FRAG_BIT_VAR0  (1 << FRAG_ATTRIB_VAR0)

#define FRAG_BIT_TEX(U)  (FRAG_BIT_TEX0 << (U))
#define FRAG_BIT_VAR(V)  (FRAG_BIT_VAR0 << (V))

#define FRAG_BITS_TEX_ANY (FRAG_BIT_TEX0|	\
			   FRAG_BIT_TEX1|	\
			   FRAG_BIT_TEX2|	\
			   FRAG_BIT_TEX3|	\
			   FRAG_BIT_TEX4|	\
			   FRAG_BIT_TEX5|	\
			   FRAG_BIT_TEX6|	\
			   FRAG_BIT_TEX7)
/*@}*/


/**
 * Fragment program results
 */
enum
{
   FRAG_RESULT_COLR = 0,
   FRAG_RESULT_COLH = 1,
   FRAG_RESULT_DEPR = 2,
   FRAG_RESULT_DATA0 = 3,
   FRAG_RESULT_MAX = (FRAG_RESULT_DATA0 + MAX_DRAW_BUFFERS)
};


/**
 * Indexes for all renderbuffers
 */
enum {
   BUFFER_FRONT_LEFT  = 0,  /* the four standard color buffers */
   BUFFER_BACK_LEFT   = 1,
   BUFFER_FRONT_RIGHT = 2,
   BUFFER_BACK_RIGHT  = 3,
   BUFFER_AUX0        = 4,  /* optional aux buffer */
   BUFFER_AUX1        = 5,
   BUFFER_AUX2        = 6,
   BUFFER_AUX3        = 7,
   BUFFER_DEPTH       = 8,
   BUFFER_STENCIL     = 9,
   BUFFER_ACCUM       = 10,
   BUFFER_COLOR0      = 11, /* generic renderbuffers */
   BUFFER_COLOR1      = 12,
   BUFFER_COLOR2      = 13,
   BUFFER_COLOR3      = 14,
   BUFFER_COLOR4      = 15,
   BUFFER_COLOR5      = 16,
   BUFFER_COLOR6      = 17,
   BUFFER_COLOR7      = 18,
   BUFFER_COUNT       = 19
};

/**
 * Bit flags for all renderbuffers
 */
#define BUFFER_BIT_FRONT_LEFT   (1 << BUFFER_FRONT_LEFT)
#define BUFFER_BIT_BACK_LEFT    (1 << BUFFER_BACK_LEFT)
#define BUFFER_BIT_FRONT_RIGHT  (1 << BUFFER_FRONT_RIGHT)
#define BUFFER_BIT_BACK_RIGHT   (1 << BUFFER_BACK_RIGHT)
#define BUFFER_BIT_AUX0         (1 << BUFFER_AUX0)
#define BUFFER_BIT_AUX1         (1 << BUFFER_AUX1)
#define BUFFER_BIT_AUX2         (1 << BUFFER_AUX2)
#define BUFFER_BIT_AUX3         (1 << BUFFER_AUX3)
#define BUFFER_BIT_DEPTH        (1 << BUFFER_DEPTH)
#define BUFFER_BIT_STENCIL      (1 << BUFFER_STENCIL)
#define BUFFER_BIT_ACCUM        (1 << BUFFER_ACCUM)
#define BUFFER_BIT_COLOR0       (1 << BUFFER_COLOR0)
#define BUFFER_BIT_COLOR1       (1 << BUFFER_COLOR1)
#define BUFFER_BIT_COLOR2       (1 << BUFFER_COLOR2)
#define BUFFER_BIT_COLOR3       (1 << BUFFER_COLOR3)
#define BUFFER_BIT_COLOR4       (1 << BUFFER_COLOR4)
#define BUFFER_BIT_COLOR5       (1 << BUFFER_COLOR5)
#define BUFFER_BIT_COLOR6       (1 << BUFFER_COLOR6)
#define BUFFER_BIT_COLOR7       (1 << BUFFER_COLOR7)

/**
 * Mask of all the color buffer bits (but not accum).
 */
#define BUFFER_BITS_COLOR  (BUFFER_BIT_FRONT_LEFT | \
                            BUFFER_BIT_BACK_LEFT | \
                            BUFFER_BIT_FRONT_RIGHT | \
                            BUFFER_BIT_BACK_RIGHT | \
                            BUFFER_BIT_AUX0 | \
                            BUFFER_BIT_AUX1 | \
                            BUFFER_BIT_AUX2 | \
                            BUFFER_BIT_AUX3 | \
                            BUFFER_BIT_COLOR0 | \
                            BUFFER_BIT_COLOR1 | \
                            BUFFER_BIT_COLOR2 | \
                            BUFFER_BIT_COLOR3 | \
                            BUFFER_BIT_COLOR4 | \
                            BUFFER_BIT_COLOR5 | \
                            BUFFER_BIT_COLOR6 | \
                            BUFFER_BIT_COLOR7)


/** The pixel transfer path has three color tables: */
/*@{*/
#define COLORTABLE_PRECONVOLUTION  0
#define COLORTABLE_POSTCONVOLUTION 1
#define COLORTABLE_POSTCOLORMATRIX 2
#define COLORTABLE_MAX 3
/*@}*/


/**
 * Data structure for color tables
 */
struct gl_color_table
{
   GLenum InternalFormat;      /**< The user-specified format */
   GLenum _BaseFormat;         /**< GL_ALPHA, GL_RGBA, GL_RGB, etc */
   GLuint Size;                /**< number of entries in table */
   GLfloat *TableF;            /**< Color table, floating point values */
   GLubyte *TableUB;           /**< Color table, ubyte values */
   GLubyte RedSize;
   GLubyte GreenSize;
   GLubyte BlueSize;
   GLubyte AlphaSize;
   GLubyte LuminanceSize;
   GLubyte IntensitySize;
};


/**
 * \name Bit flags used for updating material values.
 */
/*@{*/
#define MAT_ATTRIB_FRONT_AMBIENT           0 
#define MAT_ATTRIB_BACK_AMBIENT            1
#define MAT_ATTRIB_FRONT_DIFFUSE           2 
#define MAT_ATTRIB_BACK_DIFFUSE            3
#define MAT_ATTRIB_FRONT_SPECULAR          4 
#define MAT_ATTRIB_BACK_SPECULAR           5
#define MAT_ATTRIB_FRONT_EMISSION          6
#define MAT_ATTRIB_BACK_EMISSION           7
#define MAT_ATTRIB_FRONT_SHININESS         8
#define MAT_ATTRIB_BACK_SHININESS          9
#define MAT_ATTRIB_FRONT_INDEXES           10
#define MAT_ATTRIB_BACK_INDEXES            11
#define MAT_ATTRIB_MAX                     12

#define MAT_ATTRIB_AMBIENT(f)  (MAT_ATTRIB_FRONT_AMBIENT+(f))  
#define MAT_ATTRIB_DIFFUSE(f)  (MAT_ATTRIB_FRONT_DIFFUSE+(f))  
#define MAT_ATTRIB_SPECULAR(f) (MAT_ATTRIB_FRONT_SPECULAR+(f)) 
#define MAT_ATTRIB_EMISSION(f) (MAT_ATTRIB_FRONT_EMISSION+(f)) 
#define MAT_ATTRIB_SHININESS(f)(MAT_ATTRIB_FRONT_SHININESS+(f))
#define MAT_ATTRIB_INDEXES(f)  (MAT_ATTRIB_FRONT_INDEXES+(f))  

#define MAT_INDEX_AMBIENT  0
#define MAT_INDEX_DIFFUSE  1
#define MAT_INDEX_SPECULAR 2

#define MAT_BIT_FRONT_AMBIENT         (1<<MAT_ATTRIB_FRONT_AMBIENT)
#define MAT_BIT_BACK_AMBIENT          (1<<MAT_ATTRIB_BACK_AMBIENT)
#define MAT_BIT_FRONT_DIFFUSE         (1<<MAT_ATTRIB_FRONT_DIFFUSE)
#define MAT_BIT_BACK_DIFFUSE          (1<<MAT_ATTRIB_BACK_DIFFUSE)
#define MAT_BIT_FRONT_SPECULAR        (1<<MAT_ATTRIB_FRONT_SPECULAR)
#define MAT_BIT_BACK_SPECULAR         (1<<MAT_ATTRIB_BACK_SPECULAR)
#define MAT_BIT_FRONT_EMISSION        (1<<MAT_ATTRIB_FRONT_EMISSION)
#define MAT_BIT_BACK_EMISSION         (1<<MAT_ATTRIB_BACK_EMISSION)
#define MAT_BIT_FRONT_SHININESS       (1<<MAT_ATTRIB_FRONT_SHININESS)
#define MAT_BIT_BACK_SHININESS        (1<<MAT_ATTRIB_BACK_SHININESS)
#define MAT_BIT_FRONT_INDEXES         (1<<MAT_ATTRIB_FRONT_INDEXES)
#define MAT_BIT_BACK_INDEXES          (1<<MAT_ATTRIB_BACK_INDEXES)


#define FRONT_MATERIAL_BITS	(MAT_BIT_FRONT_EMISSION | 	\
				 MAT_BIT_FRONT_AMBIENT |	\
				 MAT_BIT_FRONT_DIFFUSE | 	\
				 MAT_BIT_FRONT_SPECULAR |	\
				 MAT_BIT_FRONT_SHININESS | 	\
				 MAT_BIT_FRONT_INDEXES)

#define BACK_MATERIAL_BITS	(MAT_BIT_BACK_EMISSION |	\
				 MAT_BIT_BACK_AMBIENT |		\
				 MAT_BIT_BACK_DIFFUSE |		\
				 MAT_BIT_BACK_SPECULAR |	\
				 MAT_BIT_BACK_SHININESS |	\
				 MAT_BIT_BACK_INDEXES)

#define ALL_MATERIAL_BITS	(FRONT_MATERIAL_BITS | BACK_MATERIAL_BITS)
/*@}*/


#define EXP_TABLE_SIZE 512	/**< Specular exponent lookup table sizes */
#define SHINE_TABLE_SIZE 256	/**< Material shininess lookup table sizes */

/**
 * Material shininess lookup table.
 */
struct gl_shine_tab
{
   struct gl_shine_tab *next, *prev;
   GLfloat tab[SHINE_TABLE_SIZE+1];
   GLfloat shininess;
   GLuint refcount;
};


/**
 * Light source state.
 */
struct gl_light
{
   struct gl_light *next;	/**< double linked list with sentinel */
   struct gl_light *prev;

   GLfloat Ambient[4];		/**< ambient color */
   GLfloat Diffuse[4];		/**< diffuse color */
   GLfloat Specular[4];		/**< specular color */
   GLfloat EyePosition[4];	/**< position in eye coordinates */
   GLfloat EyeDirection[4];	/**< spotlight dir in eye coordinates */
   GLfloat SpotExponent;
   GLfloat SpotCutoff;		/**< in degrees */
   GLfloat _CosCutoffNeg;	/**< = cos(SpotCutoff) */
   GLfloat _CosCutoff;		/**< = MAX(0, cos(SpotCutoff)) */
   GLfloat ConstantAttenuation;
   GLfloat LinearAttenuation;
   GLfloat QuadraticAttenuation;
   GLboolean Enabled;		/**< On/off flag */

   /** 
    * \name Derived fields
    */
   /*@{*/
   GLbitfield _Flags;		/**< State */

   GLfloat _Position[4];	/**< position in eye/obj coordinates */
   GLfloat _VP_inf_norm[3];	/**< Norm direction to infinite light */
   GLfloat _h_inf_norm[3];	/**< Norm( _VP_inf_norm + <0,0,1> ) */
   GLfloat _NormDirection[4];	/**< normalized spotlight direction */
   GLfloat _VP_inf_spot_attenuation;

   GLfloat _SpotExpTable[EXP_TABLE_SIZE][2];  /**< to replace a pow() call */
   GLfloat _MatAmbient[2][3];	/**< material ambient * light ambient */
   GLfloat _MatDiffuse[2][3];	/**< material diffuse * light diffuse */
   GLfloat _MatSpecular[2][3];	/**< material spec * light specular */
   GLfloat _dli;		/**< CI diffuse light intensity */
   GLfloat _sli;		/**< CI specular light intensity */
   /*@}*/
};


/**
 * Light model state.
 */
struct gl_lightmodel
{
   GLfloat Ambient[4];		/**< ambient color */
   GLboolean LocalViewer;	/**< Local (or infinite) view point? */
   GLboolean TwoSide;		/**< Two (or one) sided lighting? */
   GLenum ColorControl;		/**< either GL_SINGLE_COLOR
				 *    or GL_SEPARATE_SPECULAR_COLOR */
};


/**
 * Material state.
 */
struct gl_material
{
   GLfloat Attrib[MAT_ATTRIB_MAX][4];
};


/**
 * Accumulation buffer attribute group (GL_ACCUM_BUFFER_BIT)
 */
struct gl_accum_attrib
{
   GLfloat ClearColor[4];	/**< Accumulation buffer clear color */
};


/**
 * Color buffer attribute group (GL_COLOR_BUFFER_BIT).
 */
struct gl_colorbuffer_attrib
{
   GLuint ClearIndex;			/**< Index to use for glClear */
   GLclampf ClearColor[4];		/**< Color to use for glClear */

   GLuint IndexMask;			/**< Color index write mask */
   GLubyte ColorMask[4];		/**< Each flag is 0xff or 0x0 */

   GLenum DrawBuffer[MAX_DRAW_BUFFERS];	/**< Which buffer to draw into */

   /** 
    * \name alpha testing
    */
   /*@{*/
   GLboolean AlphaEnabled;		/**< Alpha test enabled flag */
   GLenum AlphaFunc;			/**< Alpha test function */
   GLclampf AlphaRef;			/**< Alpha reference value */
   /*@}*/

   /** 
    * \name Blending
    */
   /*@{*/
   GLboolean BlendEnabled;		/**< Blending enabled flag */
   GLenum BlendSrcRGB;			/**< Blending source operator */
   GLenum BlendDstRGB;			/**< Blending destination operator */
   GLenum BlendSrcA;			/**< GL_INGR_blend_func_separate */
   GLenum BlendDstA;			/**< GL_INGR_blend_func_separate */
   GLenum BlendEquationRGB;		/**< Blending equation */
   GLenum BlendEquationA;		/**< GL_EXT_blend_equation_separate */
   GLfloat BlendColor[4];		/**< Blending color */
   /*@}*/

   /** 
    * \name Logic op
    */
   /*@{*/
   GLenum LogicOp;			/**< Logic operator */
   GLboolean IndexLogicOpEnabled;	/**< Color index logic op enabled flag */
   GLboolean ColorLogicOpEnabled;	/**< RGBA logic op enabled flag */
   GLboolean _LogicOpEnabled;		/**< RGBA logic op + EXT_blend_logic_op enabled flag */
   /*@}*/

   GLboolean DitherFlag;		/**< Dither enable flag */

   GLenum ClampFragmentColor; /**< GL_TRUE, GL_FALSE or GL_FIXED_ONLY_ARB */
   GLenum ClampReadColor;     /**< GL_TRUE, GL_FALSE or GL_FIXED_ONLY_ARB */
};


/**
 * Current attribute group (GL_CURRENT_BIT).
 */
struct gl_current_attrib
{
   /**
    * \name Current vertex attributes.
    * \note Values are valid only after FLUSH_VERTICES has been called.
    * \note Index and Edgeflag current values are stored as floats in the 
    * SIX and SEVEN attribute slots.
    */
   /*@{*/
   GLfloat Attrib[VERT_ATTRIB_MAX][4];	/**< Position, color, texcoords, etc */
   /*@}*/

   /**
    * \name Current raster position attributes (always valid).
    * \note This set of attributes is very similar to the SWvertex struct.
    */
   /*@{*/
   GLfloat RasterPos[4];
   GLfloat RasterDistance;
   GLfloat RasterColor[4];
   GLfloat RasterSecondaryColor[4];
   GLfloat RasterIndex;
   GLfloat RasterTexCoords[MAX_TEXTURE_COORD_UNITS][4];
   GLboolean RasterPosValid;
   /*@}*/
};


/**
 * Depth buffer attribute group (GL_DEPTH_BUFFER_BIT).
 */
struct gl_depthbuffer_attrib
{
   GLenum Func;			/**< Function for depth buffer compare */
   GLclampd Clear;		/**< Value to clear depth buffer to */
   GLboolean Test;		/**< Depth buffering enabled flag */
   GLboolean Mask;		/**< Depth buffer writable? */
   GLboolean BoundsTest;        /**< GL_EXT_depth_bounds_test */
   GLfloat BoundsMin, BoundsMax;/**< GL_EXT_depth_bounds_test */
};


/**
 * glEnable()/glDisable() attribute group (GL_ENABLE_BIT).
 */
struct gl_enable_attrib
{
   GLboolean AlphaTest;
   GLboolean AutoNormal;
   GLboolean Blend;
   GLbitfield ClipPlanes;
   GLboolean ColorMaterial;
   GLboolean ColorTable[COLORTABLE_MAX];
   GLboolean Convolution1D;
   GLboolean Convolution2D;
   GLboolean Separable2D;
   GLboolean CullFace;
   GLboolean DepthTest;
   GLboolean Dither;
   GLboolean Fog;
   GLboolean Histogram;
   GLboolean Light[MAX_LIGHTS];
   GLboolean Lighting;
   GLboolean LineSmooth;
   GLboolean LineStipple;
   GLboolean IndexLogicOp;
   GLboolean ColorLogicOp;
   GLboolean Map1Color4;
   GLboolean Map1Index;
   GLboolean Map1Normal;
   GLboolean Map1TextureCoord1;
   GLboolean Map1TextureCoord2;
   GLboolean Map1TextureCoord3;
   GLboolean Map1TextureCoord4;
   GLboolean Map1Vertex3;
   GLboolean Map1Vertex4;
   GLboolean Map1Attrib[16];  /* GL_NV_vertex_program */
   GLboolean Map2Color4;
   GLboolean Map2Index;
   GLboolean Map2Normal;
   GLboolean Map2TextureCoord1;
   GLboolean Map2TextureCoord2;
   GLboolean Map2TextureCoord3;
   GLboolean Map2TextureCoord4;
   GLboolean Map2Vertex3;
   GLboolean Map2Vertex4;
   GLboolean Map2Attrib[16];  /* GL_NV_vertex_program */
   GLboolean MinMax;
   GLboolean Normalize;
   GLboolean PixelTexture;
   GLboolean PointSmooth;
   GLboolean PolygonOffsetPoint;
   GLboolean PolygonOffsetLine;
   GLboolean PolygonOffsetFill;
   GLboolean PolygonSmooth;
   GLboolean PolygonStipple;
   GLboolean RescaleNormals;
   GLboolean Scissor;
   GLboolean Stencil;
   GLboolean StencilTwoSide;          /* GL_EXT_stencil_two_side */
   GLboolean MultisampleEnabled;      /* GL_ARB_multisample */
   GLboolean SampleAlphaToCoverage;   /* GL_ARB_multisample */
   GLboolean SampleAlphaToOne;        /* GL_ARB_multisample */
   GLboolean SampleCoverage;          /* GL_ARB_multisample */
   GLboolean SampleCoverageInvert;    /* GL_ARB_multisample */
   GLboolean RasterPositionUnclipped; /* GL_IBM_rasterpos_clip */
   GLuint Texture[MAX_TEXTURE_IMAGE_UNITS];
   GLuint TexGen[MAX_TEXTURE_COORD_UNITS];
   /* SGI_texture_color_table */
   GLboolean TextureColorTable[MAX_TEXTURE_IMAGE_UNITS];
   /* GL_ARB_vertex_program / GL_NV_vertex_program */
   GLboolean VertexProgram;
   GLboolean VertexProgramPointSize;
   GLboolean VertexProgramTwoSide;
   /* GL_ARB_point_sprite / GL_NV_point_sprite */
   GLboolean PointSprite;
   GLboolean FragmentShaderATI;
};


/**
 * Evaluator attribute group (GL_EVAL_BIT).
 */
struct gl_eval_attrib
{
   /**
    * \name Enable bits 
    */
   /*@{*/
   GLboolean Map1Color4;
   GLboolean Map1Index;
   GLboolean Map1Normal;
   GLboolean Map1TextureCoord1;
   GLboolean Map1TextureCoord2;
   GLboolean Map1TextureCoord3;
   GLboolean Map1TextureCoord4;
   GLboolean Map1Vertex3;
   GLboolean Map1Vertex4;
   GLboolean Map1Attrib[16];  /* GL_NV_vertex_program */
   GLboolean Map2Color4;
   GLboolean Map2Index;
   GLboolean Map2Normal;
   GLboolean Map2TextureCoord1;
   GLboolean Map2TextureCoord2;
   GLboolean Map2TextureCoord3;
   GLboolean Map2TextureCoord4;
   GLboolean Map2Vertex3;
   GLboolean Map2Vertex4;
   GLboolean Map2Attrib[16];  /* GL_NV_vertex_program */
   GLboolean AutoNormal;
   /*@}*/
   
   /**
    * \name Map Grid endpoints and divisions and calculated du values
    */
   /*@{*/
   GLint MapGrid1un;
   GLfloat MapGrid1u1, MapGrid1u2, MapGrid1du;
   GLint MapGrid2un, MapGrid2vn;
   GLfloat MapGrid2u1, MapGrid2u2, MapGrid2du;
   GLfloat MapGrid2v1, MapGrid2v2, MapGrid2dv;
   /*@}*/
};


/**
 * Fog attribute group (GL_FOG_BIT).
 */
struct gl_fog_attrib
{
   GLboolean Enabled;		/**< Fog enabled flag */
   GLfloat Color[4];		/**< Fog color */
   GLfloat Density;		/**< Density >= 0.0 */
   GLfloat Start;		/**< Start distance in eye coords */
   GLfloat End;			/**< End distance in eye coords */
   GLfloat Index;		/**< Fog index */
   GLenum Mode;			/**< Fog mode */
   GLboolean ColorSumEnabled;
   GLenum FogCoordinateSource;  /**< GL_EXT_fog_coord */
   GLfloat _Scale;		/**< (End == Start) ? 1.0 : 1.0 / (End - Start) */
};


/** 
 * Hint attribute group (GL_HINT_BIT).
 * 
 * Values are always one of GL_FASTEST, GL_NICEST, or GL_DONT_CARE.
 */
struct gl_hint_attrib
{
   GLenum PerspectiveCorrection;
   GLenum PointSmooth;
   GLenum LineSmooth;
   GLenum PolygonSmooth;
   GLenum Fog;
   GLenum ClipVolumeClipping;   /**< GL_EXT_clip_volume_hint */
   GLenum TextureCompression;   /**< GL_ARB_texture_compression */
   GLenum GenerateMipmap;       /**< GL_SGIS_generate_mipmap */
   GLenum FragmentShaderDerivative; /**< GL_ARB_fragment_shader */
};


/**
 * Histogram attributes.
 */
struct gl_histogram_attrib
{
   GLuint Width;				/**< number of table entries */
   GLint Format;				/**< GL_ALPHA, GL_RGB, etc */
   GLuint Count[HISTOGRAM_TABLE_SIZE][4];	/**< the histogram */
   GLboolean Sink;				/**< terminate image transfer? */
   GLubyte RedSize;				/**< Bits per counter */
   GLubyte GreenSize;
   GLubyte BlueSize;
   GLubyte AlphaSize;
   GLubyte LuminanceSize;
};


/**
 * Color Min/max state.
 */
struct gl_minmax_attrib
{
   GLenum Format;
   GLboolean Sink;
   GLfloat Min[4], Max[4];   /**< RGBA */
};


/**
 * Image convolution state.
 */
struct gl_convolution_attrib
{
   GLenum Format;
   GLenum InternalFormat;
   GLuint Width;
   GLuint Height;
   GLfloat Filter[MAX_CONVOLUTION_WIDTH * MAX_CONVOLUTION_HEIGHT * 4];
};


/**
 * Light state flags.
 */
/*@{*/
#define LIGHT_SPOT         0x1
#define LIGHT_LOCAL_VIEWER 0x2
#define LIGHT_POSITIONAL   0x4
#define LIGHT_NEED_VERTICES (LIGHT_POSITIONAL|LIGHT_LOCAL_VIEWER)
/*@}*/


/**
 * Lighting attribute group (GL_LIGHT_BIT).
 */
struct gl_light_attrib
{
   struct gl_light Light[MAX_LIGHTS];	/**< Array of light sources */
   struct gl_lightmodel Model;		/**< Lighting model */

   /**
    * Must flush FLUSH_VERTICES before referencing:
    */
   /*@{*/
   struct gl_material Material; 	/**< Includes front & back values */
   /*@}*/

   GLboolean Enabled;			/**< Lighting enabled flag */
   GLenum ShadeModel;			/**< GL_FLAT or GL_SMOOTH */
   GLenum ColorMaterialFace;		/**< GL_FRONT, BACK or FRONT_AND_BACK */
   GLenum ColorMaterialMode;		/**< GL_AMBIENT, GL_DIFFUSE, etc */
   GLbitfield ColorMaterialBitmask;	/**< bitmask formed from Face and Mode */
   GLboolean ColorMaterialEnabled;
   GLenum ClampVertexColor;

   struct gl_light EnabledList;         /**< List sentinel */

   /** 
    * Derived state for optimizations: 
    */
   /*@{*/
   GLboolean _NeedEyeCoords;		
   GLboolean _NeedVertices;		/**< Use fast shader? */
   GLbitfield  _Flags;		        /**< LIGHT_* flags, see above */
   GLfloat _BaseColor[2][3];
   /*@}*/
};


/**
 * Line attribute group (GL_LINE_BIT).
 */
struct gl_line_attrib
{
   GLboolean SmoothFlag;	/**< GL_LINE_SMOOTH enabled? */
   GLboolean StippleFlag;	/**< GL_LINE_STIPPLE enabled? */
   GLushort StipplePattern;	/**< Stipple pattern */
   GLint StippleFactor;		/**< Stipple repeat factor */
   GLfloat Width;		/**< Line width */
};


/**
 * Display list attribute group (GL_LIST_BIT).
 */
struct gl_list_attrib
{
   GLuint ListBase;
};


/**
 * Used by device drivers to hook new commands into display lists.
 */
struct gl_list_instruction
{
   GLuint Size;
   void (*Execute)( GLcontext *ctx, void *data );
   void (*Destroy)( GLcontext *ctx, void *data );
   void (*Print)( GLcontext *ctx, void *data );
};

#define MAX_DLIST_EXT_OPCODES 16

/**
 * Used by device drivers to hook new commands into display lists.
 */
struct gl_list_extensions
{
   struct gl_list_instruction Opcode[MAX_DLIST_EXT_OPCODES];
   GLuint NumOpcodes;
};


/**
 * Multisample attribute group (GL_MULTISAMPLE_BIT).
 */
struct gl_multisample_attrib
{
   GLboolean Enabled;
   GLboolean _Enabled;   /**< true if Enabled and multisample buffer */
   GLboolean SampleAlphaToCoverage;
   GLboolean SampleAlphaToOne;
   GLboolean SampleCoverage;
   GLfloat SampleCoverageValue;
   GLboolean SampleCoverageInvert;
};


/**
 * A pixelmap (see glPixelMap)
 */
struct gl_pixelmap
{
   GLint Size;
   GLfloat Map[MAX_PIXEL_MAP_TABLE];
   GLubyte Map8[MAX_PIXEL_MAP_TABLE];  /**< converted to 8-bit color */
};


/**
 * Collection of all pixelmaps
 */
struct gl_pixelmaps
{
   struct gl_pixelmap RtoR;  /**< i.e. GL_PIXEL_MAP_R_TO_R */
   struct gl_pixelmap GtoG;
   struct gl_pixelmap BtoB;
   struct gl_pixelmap AtoA;
   struct gl_pixelmap ItoR;
   struct gl_pixelmap ItoG;
   struct gl_pixelmap ItoB;
   struct gl_pixelmap ItoA;
   struct gl_pixelmap ItoI;
   struct gl_pixelmap StoS;
};


/**
 * Pixel attribute group (GL_PIXEL_MODE_BIT).
 */
struct gl_pixel_attrib
{
   GLenum ReadBuffer;		/**< source buffer for glRead/CopyPixels() */

   /*--- Begin Pixel Transfer State ---*/
   /* Fields are in the order in which they're applied... */

   /* Scale & Bias (index shift, offset) */
   GLfloat RedBias, RedScale;
   GLfloat GreenBias, GreenScale;
   GLfloat BlueBias, BlueScale;
   GLfloat AlphaBias, AlphaScale;
   GLfloat DepthBias, DepthScale;
   GLint IndexShift, IndexOffset;

   /* Pixel Maps */
   /* Note: actual pixel maps are not part of this attrib group */
   GLboolean MapColorFlag;
   GLboolean MapStencilFlag;

   /* There are multiple color table stages: */
   GLboolean ColorTableEnabled[COLORTABLE_MAX];
   GLfloat ColorTableScale[COLORTABLE_MAX][4];  /**< RGBA */
   GLfloat ColorTableBias[COLORTABLE_MAX][4];   /**< RGBA */

   /* Convolution (GL_EXT_convolution) */
   GLboolean Convolution1DEnabled;
   GLboolean Convolution2DEnabled;
   GLboolean Separable2DEnabled;
   GLfloat ConvolutionBorderColor[3][4];
   GLenum ConvolutionBorderMode[3];
   GLfloat ConvolutionFilterScale[3][4];  /**< RGBA */
   GLfloat ConvolutionFilterBias[3][4];   /**< RGBA */
   GLfloat PostConvolutionScale[4];  /**< RGBA */
   GLfloat PostConvolutionBias[4];   /**< RGBA */

   /* Color matrix (GL_SGI_color_matrix) */
   /* Note: the color matrix is not part of this attrib group */
   GLfloat PostColorMatrixScale[4];  /**< RGBA */
   GLfloat PostColorMatrixBias[4];   /**< RGBA */

   /* Histogram & minmax (GL_EXT_histogram) */
   /* Note: histogram and minmax data are not part of this attrib group */
   GLboolean HistogramEnabled;
   GLboolean MinMaxEnabled;

   /*--- End Pixel Transfer State ---*/

   /* Pixel Zoom */
   GLfloat ZoomX, ZoomY;

   /** GL_SGI_texture_color_table */
   GLfloat TextureColorTableScale[4];
   GLfloat TextureColorTableBias[4];
};


/**
 * Point attribute group (GL_POINT_BIT).
 */
struct gl_point_attrib
{
   GLboolean SmoothFlag;	/**< True if GL_POINT_SMOOTH is enabled */
   GLfloat Size;		/**< User-specified point size */
   GLfloat Params[3];		/**< GL_EXT_point_parameters */
   GLfloat MinSize, MaxSize;	/**< GL_EXT_point_parameters */
   GLfloat Threshold;		/**< GL_EXT_point_parameters */
   GLboolean _Attenuated;	/**< True if Params != [1, 0, 0] */
   GLboolean PointSprite;	/**< GL_NV/ARB_point_sprite */
   GLboolean CoordReplace[MAX_TEXTURE_COORD_UNITS]; /**< GL_ARB_point_sprite */
   GLenum SpriteRMode;		/**< GL_NV_point_sprite (only!) */
   GLenum SpriteOrigin;		/**< GL_ARB_point_sprite */
};


/**
 * Polygon attribute group (GL_POLYGON_BIT).
 */
struct gl_polygon_attrib
{
   GLenum FrontFace;		/**< Either GL_CW or GL_CCW */
   GLenum FrontMode;		/**< Either GL_POINT, GL_LINE or GL_FILL */
   GLenum BackMode;		/**< Either GL_POINT, GL_LINE or GL_FILL */
   GLboolean _FrontBit;		/**< 0=GL_CCW, 1=GL_CW */
   GLboolean CullFlag;		/**< Culling on/off flag */
   GLboolean SmoothFlag;	/**< True if GL_POLYGON_SMOOTH is enabled */
   GLboolean StippleFlag;	/**< True if GL_POLYGON_STIPPLE is enabled */
   GLenum CullFaceMode;		/**< Culling mode GL_FRONT or GL_BACK */
   GLfloat OffsetFactor;	/**< Polygon offset factor, from user */
   GLfloat OffsetUnits;		/**< Polygon offset units, from user */
   GLboolean OffsetPoint;	/**< Offset in GL_POINT mode */
   GLboolean OffsetLine;	/**< Offset in GL_LINE mode */
   GLboolean OffsetFill;	/**< Offset in GL_FILL mode */
};


/**
 * Scissor attributes (GL_SCISSOR_BIT).
 */
struct gl_scissor_attrib
{
   GLboolean Enabled;		/**< Scissor test enabled? */
   GLint X, Y;			/**< Lower left corner of box */
   GLsizei Width, Height;	/**< Size of box */
};


/**
 * Stencil attribute group (GL_STENCIL_BUFFER_BIT).
 */
struct gl_stencil_attrib
{
   GLboolean Enabled;		/**< Enabled flag */
   GLboolean TestTwoSide;	/**< GL_EXT_stencil_two_side */
   GLubyte ActiveFace;		/**< GL_EXT_stencil_two_side (0 or 1) */
   GLboolean _TestTwoSide;
   GLenum Function[2];		/**< Stencil function */
   GLenum FailFunc[2];		/**< Fail function */
   GLenum ZPassFunc[2];		/**< Depth buffer pass function */
   GLenum ZFailFunc[2];		/**< Depth buffer fail function */
   GLint Ref[2];		/**< Reference value */
   GLuint ValueMask[2];		/**< Value mask */
   GLuint WriteMask[2];		/**< Write mask */
   GLuint Clear;		/**< Clear value */
};


#define NUM_TEXTURE_TARGETS 7   /* 1D, 2D, 3D, CUBE, RECT, 1D_STACK, and 2D_STACK */

/**
 * An index for each type of texture object
 */
/*@{*/
#define TEXTURE_1D_INDEX       0
#define TEXTURE_2D_INDEX       1
#define TEXTURE_3D_INDEX       2
#define TEXTURE_CUBE_INDEX     3
#define TEXTURE_RECT_INDEX     4
#define TEXTURE_1D_ARRAY_INDEX 5
#define TEXTURE_2D_ARRAY_INDEX 6
/*@}*/

/**
 * Bit flags for each type of texture object
 * Used for Texture.Unit[]._ReallyEnabled flags.
 */
/*@{*/
#define TEXTURE_1D_BIT       (1 << TEXTURE_1D_INDEX)
#define TEXTURE_2D_BIT       (1 << TEXTURE_2D_INDEX)
#define TEXTURE_3D_BIT       (1 << TEXTURE_3D_INDEX)
#define TEXTURE_CUBE_BIT     (1 << TEXTURE_CUBE_INDEX)
#define TEXTURE_RECT_BIT     (1 << TEXTURE_RECT_INDEX)
#define TEXTURE_1D_ARRAY_BIT (1 << TEXTURE_1D_ARRAY_INDEX)
#define TEXTURE_2D_ARRAY_BIT (1 << TEXTURE_2D_ARRAY_INDEX)
/*@}*/


/**
 * TexGenEnabled flags.
 */
/*@{*/
#define S_BIT 1
#define T_BIT 2
#define R_BIT 4
#define Q_BIT 8
/*@}*/


/**
 * Bit flag versions of the corresponding GL_ constants.
 */
/*@{*/
#define TEXGEN_SPHERE_MAP        0x1
#define TEXGEN_OBJ_LINEAR        0x2
#define TEXGEN_EYE_LINEAR        0x4
#define TEXGEN_REFLECTION_MAP_NV 0x8
#define TEXGEN_NORMAL_MAP_NV     0x10

#define TEXGEN_NEED_NORMALS      (TEXGEN_SPHERE_MAP        | \
				  TEXGEN_REFLECTION_MAP_NV | \
				  TEXGEN_NORMAL_MAP_NV)
#define TEXGEN_NEED_EYE_COORD    (TEXGEN_SPHERE_MAP        | \
				  TEXGEN_REFLECTION_MAP_NV | \
				  TEXGEN_NORMAL_MAP_NV     | \
				  TEXGEN_EYE_LINEAR)
/*@}*/


/* A selection of state flags to make driver and module's lives easier. */
#define ENABLE_TEXGEN0        0x1
#define ENABLE_TEXGEN1        0x2
#define ENABLE_TEXGEN2        0x4
#define ENABLE_TEXGEN3        0x8
#define ENABLE_TEXGEN4        0x10
#define ENABLE_TEXGEN5        0x20
#define ENABLE_TEXGEN6        0x40
#define ENABLE_TEXGEN7        0x80

#define ENABLE_TEXMAT0        0x1	/* Ie. not the identity matrix */
#define ENABLE_TEXMAT1        0x2
#define ENABLE_TEXMAT2        0x4
#define ENABLE_TEXMAT3        0x8
#define ENABLE_TEXMAT4        0x10
#define ENABLE_TEXMAT5        0x20
#define ENABLE_TEXMAT6        0x40
#define ENABLE_TEXMAT7        0x80

#define ENABLE_TEXGEN(i) (ENABLE_TEXGEN0 << (i))
#define ENABLE_TEXMAT(i) (ENABLE_TEXMAT0 << (i))


/**
 * Texel fetch function prototype.  We use texel fetch functions to
 * extract RGBA, color indexes and depth components out of 1D, 2D and 3D
 * texture images.  These functions help to isolate us from the gritty
 * details of all the various texture image encodings.
 * 
 * \param texImage texture image.
 * \param col texel column.
 * \param row texel row.
 * \param img texel image level/layer.
 * \param texelOut output texel (up to 4 GLchans)
 */
typedef void (*FetchTexelFuncC)( const struct gl_texture_image *texImage,
                                 GLint col, GLint row, GLint img,
                                 GLchan *texelOut );

/**
 * As above, but returns floats.
 * Used for depth component images and for upcoming signed/float
 * texture images.
 */
typedef void (*FetchTexelFuncF)( const struct gl_texture_image *texImage,
                                 GLint col, GLint row, GLint img,
                                 GLfloat *texelOut );


typedef void (*StoreTexelFunc)(struct gl_texture_image *texImage,
                               GLint col, GLint row, GLint img,
                               const void *texel);


/**
 * This macro defines the (many) parameters to the texstore functions.
 * \param dims  either 1 or 2 or 3
 * \param baseInternalFormat  user-specified base internal format
 * \param dstFormat  destination Mesa texture format
 * \param dstAddr  destination image address
 * \param dstX/Y/Zoffset  destination x/y/z offset (ala TexSubImage), in texels
 * \param dstRowStride  destination image row stride, in bytes
 * \param dstImageOffsets  offset of each 2D slice within 3D texture, in texels
 * \param srcWidth/Height/Depth  source image size, in pixels
 * \param srcFormat  incoming image format
 * \param srcType  incoming image data type
 * \param srcAddr  source image address
 * \param srcPacking  source image packing parameters
 */
#define TEXSTORE_PARAMS \
	GLcontext *ctx, GLuint dims, \
	GLenum baseInternalFormat, \
	const struct gl_texture_format *dstFormat, \
	GLvoid *dstAddr, \
	GLint dstXoffset, GLint dstYoffset, GLint dstZoffset, \
	GLint dstRowStride, const GLuint *dstImageOffsets, \
	GLint srcWidth, GLint srcHeight, GLint srcDepth, \
	GLenum srcFormat, GLenum srcType, \
	const GLvoid *srcAddr, \
	const struct gl_pixelstore_attrib *srcPacking



/**
 * Texture image storage function.
 */
typedef GLboolean (*StoreTexImageFunc)(TEXSTORE_PARAMS);


/**
 * Texture format record 
 */
struct gl_texture_format
{
   GLint MesaFormat;		/**< One of the MESA_FORMAT_* values */

   GLenum BaseFormat;		/**< Either GL_RGB, GL_RGBA, GL_ALPHA,
				 *   GL_LUMINANCE, GL_LUMINANCE_ALPHA,
				 *   GL_INTENSITY, GL_COLOR_INDEX or
				 *   GL_DEPTH_COMPONENT.
				 */
   GLenum DataType;		/**< GL_FLOAT or GL_UNSIGNED_NORMALIZED_ARB */

   /**
    * Bits per texel component.  These are just rough approximations
    * for compressed texture formats.
    */
   /*@{*/
   GLubyte RedBits;
   GLubyte GreenBits;
   GLubyte BlueBits;
   GLubyte AlphaBits;
   GLubyte LuminanceBits;
   GLubyte IntensityBits;
   GLubyte IndexBits;
   GLubyte DepthBits;
   GLubyte StencilBits; 	/**< GL_EXT_packed_depth_stencil */
   /*@}*/

   GLuint TexelBytes;		/**< Bytes per texel, 0 if compressed format */

   StoreTexImageFunc StoreImage;

   /**
    * \name Texel fetch function pointers
    */
   /*@{*/
   FetchTexelFuncC FetchTexel1D;
   FetchTexelFuncC FetchTexel2D;
   FetchTexelFuncC FetchTexel3D;
   FetchTexelFuncF FetchTexel1Df;
   FetchTexelFuncF FetchTexel2Df;
   FetchTexelFuncF FetchTexel3Df;
   /*@}*/

   StoreTexelFunc StoreTexel;
};


/**
 * Texture image state.  Describes the dimensions of a texture image,
 * the texel format and pointers to Texel Fetch functions.
 */
struct gl_texture_image
{
   GLenum _BaseFormat;		/**< Either GL_RGB, GL_RGBA, GL_ALPHA,
				 *   GL_LUMINANCE, GL_LUMINANCE_ALPHA,
				 *   GL_INTENSITY, GL_COLOR_INDEX,
				 *   GL_DEPTH_COMPONENT or GL_DEPTH_STENCIL_EXT
                                 *   only. Used for choosing TexEnv arithmetic.
				 */
   GLint InternalFormat;	/**< Internal format as given by the user */
   GLuint Border;		/**< 0 or 1 */
   GLuint Width;		/**< = 2^WidthLog2 + 2*Border */
   GLuint Height;		/**< = 2^HeightLog2 + 2*Border */
   GLuint Depth;		/**< = 2^DepthLog2 + 2*Border */
   GLuint Width2;		/**< = Width - 2*Border */
   GLuint Height2;		/**< = Height - 2*Border */
   GLuint Depth2;		/**< = Depth - 2*Border */
   GLuint WidthLog2;		/**< = log2(Width2) */
   GLuint HeightLog2;		/**< = log2(Height2) */
   GLuint DepthLog2;		/**< = log2(Depth2) */
   GLuint MaxLog2;		/**< = MAX(WidthLog2, HeightLog2) */
   GLfloat WidthScale;		/**< used for mipmap LOD computation */
   GLfloat HeightScale;		/**< used for mipmap LOD computation */
   GLfloat DepthScale;		/**< used for mipmap LOD computation */
   GLboolean IsClientData;	/**< Data owned by client? */
   GLboolean _IsPowerOfTwo;	/**< Are all dimensions powers of two? */

   const struct gl_texture_format *TexFormat;

   struct gl_texture_object *TexObject;  /**< Pointer back to parent object */

   FetchTexelFuncC FetchTexelc;	/**< GLchan texel fetch function pointer */
   FetchTexelFuncF FetchTexelf;	/**< Float texel fetch function pointer */

   GLboolean IsCompressed;	/**< GL_ARB_texture_compression */
   GLuint CompressedSize;	/**< GL_ARB_texture_compression */

   GLuint RowStride;		/**< Padded width in units of texels */
   GLuint *ImageOffsets;        /**< if 3D texture: array [Depth] of offsets to
                                     each 2D slice in 'Data', in texels */
   GLvoid *Data;		/**< Image data, accessed via FetchTexel() */

   /**
    * \name For device driver:
    */
   /*@{*/
   void *DriverData;		/**< Arbitrary device driver data */
   /*@}*/
};


/**
 * Indexes for cube map faces.
 */
/*@{*/
#define FACE_POS_X   0
#define FACE_NEG_X   1
#define FACE_POS_Y   2
#define FACE_NEG_Y   3
#define FACE_POS_Z   4
#define FACE_NEG_Z   5
#define MAX_FACES    6
/*@}*/


/**
 * Texture object state.  Contains the array of mipmap images, border color,
 * wrap modes, filter modes, shadow/texcompare state, and the per-texture
 * color palette.
 */
struct gl_texture_object
{
   _glthread_Mutex Mutex;	/**< for thread safety */
   GLint RefCount;		/**< reference count */
   GLuint Name;			/**< the user-visible texture object ID */
   GLenum Target;               /**< GL_TEXTURE_1D, GL_TEXTURE_2D, etc. */
   GLfloat Priority;		/**< in [0,1] */
   GLfloat BorderColor[4];	/**< unclamped */
   GLchan _BorderChan[4];	/**< clamped, as GLchan */
   GLenum WrapS;		/**< S-axis texture image wrap mode */
   GLenum WrapT;		/**< T-axis texture image wrap mode */
   GLenum WrapR;		/**< R-axis texture image wrap mode */
   GLenum MinFilter;		/**< minification filter */
   GLenum MagFilter;		/**< magnification filter */
   GLfloat MinLod;		/**< min lambda, OpenGL 1.2 */
   GLfloat MaxLod;		/**< max lambda, OpenGL 1.2 */
   GLfloat LodBias;		/**< OpenGL 1.4 */
   GLint BaseLevel;		/**< min mipmap level, OpenGL 1.2 */
   GLint MaxLevel;		/**< max mipmap level, OpenGL 1.2 */
   GLfloat MaxAnisotropy;	/**< GL_EXT_texture_filter_anisotropic */
   GLboolean CompareFlag;	/**< GL_SGIX_shadow */
   GLenum CompareOperator;	/**< GL_SGIX_shadow */
   GLfloat ShadowAmbient;       /**< GL_ARB_shadow_ambient */
   GLenum CompareMode;		/**< GL_ARB_shadow */
   GLenum CompareFunc;		/**< GL_ARB_shadow */
   GLenum _Function;		/**< Comparison function derived from 
				 * \c CompareOperator, \c CompareMode, and
				 * \c CompareFunc.
				 */
   GLenum DepthMode;		/**< GL_ARB_depth_texture */
   GLint _MaxLevel;		/**< actual max mipmap level (q in the spec) */
   GLfloat _MaxLambda;		/**< = _MaxLevel - BaseLevel (q - b in spec) */
   GLboolean GenerateMipmap;    /**< GL_SGIS_generate_mipmap */
   GLboolean _Complete;		/**< Is texture object complete? */

   /** Actual texture images, indexed by [cube face] and [mipmap level] */
   struct gl_texture_image *Image[MAX_FACES][MAX_TEXTURE_LEVELS];

   /** GL_EXT_paletted_texture */
   struct gl_color_table Palette;


   /**
    * \name For device driver.
    * Note: instead of attaching driver data to this pointer, it's preferable
    * to instead use this struct as a base class for your own texture object
    * class.  Driver->NewTextureObject() can be used to implement the
    * allocation.
    */
   void *DriverData;	/**< Arbitrary device driver data */
};


/**
 * Texture combine environment state.
 * 
 * \todo
 * If GL_NV_texture_env_combine4 is ever supported, the arrays in this
 * structure will need to be expanded for 4 elements.
 */
struct gl_tex_env_combine_state
{
   GLenum ModeRGB;       /**< GL_REPLACE, GL_DECAL, GL_ADD, etc. */
   GLenum ModeA;         /**< GL_REPLACE, GL_DECAL, GL_ADD, etc. */
   GLenum SourceRGB[3];  /**< GL_PRIMARY_COLOR, GL_TEXTURE, etc. */
   GLenum SourceA[3];    /**< GL_PRIMARY_COLOR, GL_TEXTURE, etc. */
   GLenum OperandRGB[3]; /**< SRC_COLOR, ONE_MINUS_SRC_COLOR, etc */
   GLenum OperandA[3];   /**< SRC_ALPHA, ONE_MINUS_SRC_ALPHA, etc */
   GLuint ScaleShiftRGB; /**< 0, 1 or 2 */
   GLuint ScaleShiftA;   /**< 0, 1 or 2 */
   GLuint _NumArgsRGB;   /**< Number of inputs used for the combine mode. */
   GLuint _NumArgsA;     /**< Number of inputs used for the combine mode. */
};


/**
 * Texture unit state.  Contains enable flags, texture environment/function/
 * combiners, texgen state, pointers to current texture objects and
 * post-filter color tables.
 */
struct gl_texture_unit
{
   GLbitfield Enabled;          /**< bitmask of TEXTURE_*_BIT flags */
   GLbitfield _ReallyEnabled;   /**< 0 or exactly one of TEXTURE_*_BIT flags */

   GLenum EnvMode;              /**< GL_MODULATE, GL_DECAL, GL_BLEND, etc. */
   GLfloat EnvColor[4];
   GLbitfield TexGenEnabled;	/**< Bitwise-OR of [STRQ]_BIT values */
   /** \name Tex coord generation mode
    * Either GL_OBJECT_LINEAR, GL_EYE_LINEAR or GL_SPHERE_MAP. */
   /*@{*/
   GLenum GenModeS;		
   GLenum GenModeT;
   GLenum GenModeR;
   GLenum GenModeQ;
   /*@}*/
   GLbitfield _GenBitS;
   GLbitfield _GenBitT;
   GLbitfield _GenBitR;
   GLbitfield _GenBitQ;
   GLbitfield _GenFlags;	/**< bitwise or of _GenBit[STRQ] */
   GLfloat ObjectPlaneS[4];
   GLfloat ObjectPlaneT[4];
   GLfloat ObjectPlaneR[4];
   GLfloat ObjectPlaneQ[4];
   GLfloat EyePlaneS[4];
   GLfloat EyePlaneT[4];
   GLfloat EyePlaneR[4];
   GLfloat EyePlaneQ[4];
   GLfloat LodBias;		/**< for biasing mipmap levels */

   /** 
    * \name GL_EXT_texture_env_combine 
    */
   struct gl_tex_env_combine_state Combine;

   /**
    * Derived state based on \c EnvMode and the \c BaseFormat of the
    * currently enabled texture.
    */
   struct gl_tex_env_combine_state _EnvMode;

   /**
    * Currently enabled combiner state.  This will point to either
    * \c Combine or \c _EnvMode.
    */
   struct gl_tex_env_combine_state *_CurrentCombine;

   struct gl_texture_object *Current1D;
   struct gl_texture_object *Current2D;
   struct gl_texture_object *Current3D;
   struct gl_texture_object *CurrentCubeMap; /**< GL_ARB_texture_cube_map */
   struct gl_texture_object *CurrentRect;    /**< GL_NV_texture_rectangle */
   struct gl_texture_object *Current1DArray; /**< GL_MESA_texture_array */
   struct gl_texture_object *Current2DArray; /**< GL_MESA_texture_array */

   struct gl_texture_object *_Current; /**< Points to really enabled tex obj */

   /** GL_SGI_texture_color_table */
   /*@{*/
   struct gl_color_table ColorTable;
   struct gl_color_table ProxyColorTable;
   GLboolean ColorTableEnabled;
   /*@}*/
};


struct texenvprog_cache_item;

struct texenvprog_cache
{
   struct texenvprog_cache_item **items;
   GLuint size, n_items;
   GLcontext *ctx;
};


/**
 * Texture attribute group (GL_TEXTURE_BIT).
 */
struct gl_texture_attrib
{
   /**
    * name multitexture 
    */
   /**@{*/
   GLuint CurrentUnit;	       /**< Active texture unit */
   GLbitfield _EnabledUnits;  /**< one bit set for each really-enabled unit */
   GLbitfield _EnabledCoordUnits;   /**< one bit per enabled coordinate unit */
   GLbitfield _GenFlags;            /**< for texgen */
   GLbitfield _TexGenEnabled;
   GLbitfield _TexMatEnabled;
   /**@}*/

   struct gl_texture_unit Unit[MAX_TEXTURE_UNITS];

   /** Proxy texture objects */
   struct gl_texture_object *ProxyTex[NUM_TEXTURE_TARGETS];

   /** GL_EXT_shared_texture_palette */
   GLboolean SharedPalette;
   struct gl_color_table Palette;
   
   /** Cached texenv fragment programs */
   struct texenvprog_cache env_fp_cache;
};


/**
 * Transformation attribute group (GL_TRANSFORM_BIT).
 */
struct gl_transform_attrib
{
   GLenum MatrixMode;				/**< Matrix mode */
   GLfloat EyeUserPlane[MAX_CLIP_PLANES][4];	/**< User clip planes */
   GLfloat _ClipUserPlane[MAX_CLIP_PLANES][4];	/**< derived */
   GLbitfield ClipPlanesEnabled;                /**< on/off bitmask */
   GLboolean Normalize;				/**< Normalize all normals? */
   GLboolean RescaleNormals;			/**< GL_EXT_rescale_normal */
   GLboolean RasterPositionUnclipped;           /**< GL_IBM_rasterpos_clip */

   GLboolean CullVertexFlag;	/**< True if GL_CULL_VERTEX_EXT is enabled */
   GLfloat CullEyePos[4];
   GLfloat CullObjPos[4];
};


/**
 * Viewport attribute group (GL_VIEWPORT_BIT).
 */
struct gl_viewport_attrib
{
   GLint X, Y;			/**< position */
   GLsizei Width, Height;	/**< size */
   GLfloat Near, Far;		/**< Depth buffer range */
   GLmatrix _WindowMap;		/**< Mapping transformation as a matrix. */
};


/**
 * Node for the attribute stack.
 */
struct gl_attrib_node
{
   GLbitfield kind;
   void *data;
   struct gl_attrib_node *next;
};


/**
 * GL_ARB_vertex/pixel_buffer_object buffer object
 */
struct gl_buffer_object
{
   GLint RefCount;
   GLuint Name;
   GLenum Usage;
   GLenum Access;
   GLvoid *Pointer;          /**< Only valid while buffer is mapped */
   GLsizeiptrARB Size;       /**< Size of storage in bytes */
   GLubyte *Data;            /**< Location of storage either in RAM or VRAM. */
   GLboolean OnCard;         /**< Is buffer in VRAM? (hardware drivers) */
};



/**
 * Client pixel packing/unpacking attributes
 */
struct gl_pixelstore_attrib
{
   GLint Alignment;
   GLint RowLength;
   GLint SkipPixels;
   GLint SkipRows;
   GLint ImageHeight;     /**< for GL_EXT_texture3D */
   GLint SkipImages;      /**< for GL_EXT_texture3D */
   GLboolean SwapBytes;
   GLboolean LsbFirst;
   GLboolean ClientStorage; /**< GL_APPLE_client_storage */
   GLboolean Invert;        /**< GL_MESA_pack_invert */
   struct gl_buffer_object *BufferObj; /**< GL_ARB_pixel_buffer_object */
};



/**
 * Client vertex array attributes
 */
struct gl_client_array
{
   GLint Size;                  /**< components per element (1,2,3,4) */
   GLenum Type;                 /**< datatype: GL_FLOAT, GL_INT, etc */
   GLsizei Stride;		/**< user-specified stride */
   GLsizei StrideB;		/**< actual stride in bytes */
   const GLubyte *Ptr;          /**< Points to array data */
   GLboolean Enabled;		/**< Enabled flag is a boolean */
   GLboolean Normalized;        /**< GL_ARB_vertex_program */

   /**< GL_ARB_vertex_buffer_object */
   struct gl_buffer_object *BufferObj;
   GLuint _MaxElement;
};


/**
 * Collection of vertex arrays.  Defined by the GL_APPLE_vertex_array_object
 * extension, but a nice encapsulation in any case.
 */
struct gl_array_object
{
   /** Name of the array object as received from glGenVertexArrayAPPLE. */
   GLuint Name;

   /** Conventional vertex arrays */
   /*@{*/
   struct gl_client_array Vertex;
   struct gl_client_array Normal;
   struct gl_client_array Color;
   struct gl_client_array SecondaryColor;
   struct gl_client_array FogCoord;
   struct gl_client_array Index;
   struct gl_client_array EdgeFlag;
   struct gl_client_array TexCoord[MAX_TEXTURE_COORD_UNITS];
   /*@}*/

   /** Generic arrays for vertex programs/shaders */
   struct gl_client_array VertexAttrib[VERT_ATTRIB_MAX];

   /** Mask of _NEW_ARRAY_* values indicating which arrays are enabled */
   GLbitfield _Enabled;
};


/**
 * Vertex array state
 */
struct gl_array_attrib
{
   struct gl_array_object *ArrayObj;
   struct gl_array_object *DefaultArrayObj;

   GLint ActiveTexture;		/**< Client Active Texture */
   GLuint LockFirst;            /**< GL_EXT_compiled_vertex_array */
   GLuint LockCount;            /**< GL_EXT_compiled_vertex_array */

   GLbitfield NewState;		/**< mask of _NEW_ARRAY_* values */

#if FEATURE_ARB_vertex_buffer_object
   struct gl_buffer_object *NullBufferObj;
   struct gl_buffer_object *ArrayBufferObj;
   struct gl_buffer_object *ElementArrayBufferObj;
#endif
   GLuint _MaxElement;          /* Min of all enabled array's maxes */
};


/**
 * Feedback buffer state
 */
struct gl_feedback
{
   GLenum Type;
   GLbitfield _Mask;		/* FB_* bits */
   GLfloat *Buffer;
   GLuint BufferSize;
   GLuint Count;
};


/**
 * Selection buffer state
 */
struct gl_selection
{
   GLuint *Buffer;	/**< selection buffer */
   GLuint BufferSize;	/**< size of the selection buffer */
   GLuint BufferCount;	/**< number of values in the selection buffer */
   GLuint Hits;		/**< number of records in the selection buffer */
   GLuint NameStackDepth; /**< name stack depth */
   GLuint NameStack[MAX_NAME_STACK_DEPTH]; /**< name stack */
   GLboolean HitFlag;	/**< hit flag */
   GLfloat HitMinZ;	/**< minimum hit depth */
   GLfloat HitMaxZ;	/**< maximum hit depth */
};


/**
 * 1-D Evaluator control points
 */
struct gl_1d_map
{
   GLuint Order;	/**< Number of control points */
   GLfloat u1, u2, du;	/**< u1, u2, 1.0/(u2-u1) */
   GLfloat *Points;	/**< Points to contiguous control points */
};


/**
 * 2-D Evaluator control points
 */
struct gl_2d_map
{
   GLuint Uorder;		/**< Number of control points in U dimension */
   GLuint Vorder;		/**< Number of control points in V dimension */
   GLfloat u1, u2, du;
   GLfloat v1, v2, dv;
   GLfloat *Points;		/**< Points to contiguous control points */
};


/**
 * All evaluator control point state
 */
struct gl_evaluators
{
   /** 
    * \name 1-D maps
    */
   /*@{*/
   struct gl_1d_map Map1Vertex3;
   struct gl_1d_map Map1Vertex4;
   struct gl_1d_map Map1Index;
   struct gl_1d_map Map1Color4;
   struct gl_1d_map Map1Normal;
   struct gl_1d_map Map1Texture1;
   struct gl_1d_map Map1Texture2;
   struct gl_1d_map Map1Texture3;
   struct gl_1d_map Map1Texture4;
   struct gl_1d_map Map1Attrib[16];  /**< GL_NV_vertex_program */
   /*@}*/

   /** 
    * \name 2-D maps 
    */
   /*@{*/
   struct gl_2d_map Map2Vertex3;
   struct gl_2d_map Map2Vertex4;
   struct gl_2d_map Map2Index;
   struct gl_2d_map Map2Color4;
   struct gl_2d_map Map2Normal;
   struct gl_2d_map Map2Texture1;
   struct gl_2d_map Map2Texture2;
   struct gl_2d_map Map2Texture3;
   struct gl_2d_map Map2Texture4;
   struct gl_2d_map Map2Attrib[16];  /**< GL_NV_vertex_program */
   /*@}*/
};


/**
 * Names of the various vertex/fragment program register files, etc.
 *
 * NOTE: first four tokens must fit into 2 bits (see t_vb_arbprogram.c)
 * All values should fit in a 4-bit field.
 *
 * NOTE: PROGRAM_ENV_PARAM, PROGRAM_STATE_VAR, PROGRAM_NAMED_PARAM,
 * PROGRAM_CONSTANT, and PROGRAM_UNIFORM can all be considered to
 * be "uniform" variables since they can only be set outside glBegin/End.
 * They're also all stored in the same Parameters array.
 */
enum register_file
{
   PROGRAM_TEMPORARY = 0,   /**< machine->Temporary[] */
   PROGRAM_LOCAL_PARAM = 1, /**< gl_program->LocalParams[] */
   PROGRAM_ENV_PARAM = 2,   /**< gl_program->Parameters[] */
   PROGRAM_STATE_VAR = 3,   /**< gl_program->Parameters[] */
   PROGRAM_INPUT = 4,       /**< machine->Inputs[] */
   PROGRAM_OUTPUT = 5,      /**< machine->Outputs[] */
   PROGRAM_NAMED_PARAM = 6, /**< gl_program->Parameters[] */
   PROGRAM_CONSTANT = 7,    /**< gl_program->Parameters[] */
   PROGRAM_UNIFORM = 8,     /**< gl_program->Parameters[] */
   PROGRAM_VARYING = 9,     /**< machine->Inputs[]/Outputs[] */
   PROGRAM_WRITE_ONLY = 10, /**< A dummy, write-only register */
   PROGRAM_ADDRESS = 11,    /**< machine->AddressReg */
   PROGRAM_SAMPLER = 12,    /**< for shader samplers, compile-time only */
   PROGRAM_UNDEFINED = 13,  /**< Invalid value */
   PROGRAM_FILE_MAX
};


/** Vertex and fragment instructions */
struct prog_instruction;
struct gl_program_parameter_list;
struct gl_uniform_list;


/**
 * Base class for any kind of program object
 */
struct gl_program
{
   GLuint Id;
   GLubyte *String;  /**< Null-terminated program text */
   GLint RefCount;
   GLenum Target;    /**< GL_VERTEX/FRAGMENT_PROGRAM_ARB, GL_FRAGMENT_PROGRAM_NV */
   GLenum Format;    /**< String encoding format */
   GLboolean Resident;

   struct prog_instruction *Instructions;

   GLbitfield InputsRead;     /**< Bitmask of which input regs are read */
   GLbitfield OutputsWritten; /**< Bitmask of which output regs are written to */
   GLbitfield TexturesUsed[MAX_TEXTURE_IMAGE_UNITS];  /**< TEXTURE_x_BIT bitmask */
   GLbitfield SamplersUsed;   /**< Bitfield of which samplers are used */
   GLbitfield ShadowSamplers; /**< Texture units used for shadow sampling. */

   /** Named parameters, constants, etc. from program text */
   struct gl_program_parameter_list *Parameters;
   /** Numbered local parameters */
   GLfloat LocalParams[MAX_PROGRAM_LOCAL_PARAMS][4];

   /** Vertex/fragment shader varying vars */
   struct gl_program_parameter_list *Varying;
   /** Vertex program user-defined attributes */
   struct gl_program_parameter_list *Attributes;

   /** Map from sampler unit to texture unit (set by glUniform1i()) */
   GLubyte SamplerUnits[MAX_SAMPLERS];
   /** Which texture target is being sampled (TEXTURE_1D/2D/3D/etc_INDEX) */
   GLubyte SamplerTargets[MAX_SAMPLERS];

   /** Logical counts */
   /*@{*/
   GLuint NumInstructions;
   GLuint NumTemporaries;
   GLuint NumParameters;
   GLuint NumAttributes;
   GLuint NumAddressRegs;
   GLuint NumAluInstructions;
   GLuint NumTexInstructions;
   GLuint NumTexIndirections;
   /*@}*/
   /** Native, actual h/w counts */
   /*@{*/
   GLuint NumNativeInstructions;
   GLuint NumNativeTemporaries;
   GLuint NumNativeParameters;
   GLuint NumNativeAttributes;
   GLuint NumNativeAddressRegs;
   GLuint NumNativeAluInstructions;
   GLuint NumNativeTexInstructions;
   GLuint NumNativeTexIndirections;
   /*@}*/
};


/** Vertex program object */
struct gl_vertex_program
{
   struct gl_program Base;   /**< base class */
   GLboolean IsNVProgram;    /**< is this a GL_NV_vertex_program program? */
   GLboolean IsPositionInvariant;
   void *TnlData;		/**< should probably use Base.DriverData */
};


/** Fragment program object */
struct gl_fragment_program
{
   struct gl_program Base;   /**< base class */
   GLenum FogOption;
   GLboolean UsesKill;
};


/**
 * State common to vertex and fragment programs.
 */
struct gl_program_state
{
   GLint ErrorPos;                       /* GL_PROGRAM_ERROR_POSITION_ARB/NV */
   const char *ErrorString;              /* GL_PROGRAM_ERROR_STRING_ARB/NV */
};


/**
 * Context state for vertex programs.
 */
struct gl_vertex_program_state
{
   GLboolean Enabled;               /**< GL_VERTEX_PROGRAM_ARB/NV */
   GLboolean _Enabled;              /**< Enabled and valid program? */
   GLboolean PointSizeEnabled;      /**< GL_VERTEX_PROGRAM_POINT_SIZE_ARB/NV */
   GLboolean TwoSideEnabled;        /**< GL_VERTEX_PROGRAM_TWO_SIDE_ARB/NV */
   struct gl_vertex_program *Current;  /**< user-bound vertex program */

   /** Currently enabled and valid program (including internal programs
    * and compiled shader programs).
    */
   struct gl_vertex_program *_Current;

   GLfloat Parameters[MAX_PROGRAM_ENV_PARAMS][4]; /**< Env params */

   /* For GL_NV_vertex_program only: */
   GLenum TrackMatrix[MAX_PROGRAM_ENV_PARAMS / 4];
   GLenum TrackMatrixTransform[MAX_PROGRAM_ENV_PARAMS / 4];

   /** Should fixed-function T&L be implemented with a vertex prog? */
   GLboolean _MaintainTnlProgram;

   /** Program to emulate fixed-function T&L (see above) */
   struct gl_vertex_program *_TnlProgram;

#if FEATURE_MESA_program_debug
   GLprogramcallbackMESA Callback;
   GLvoid *CallbackData;
   GLboolean CallbackEnabled;
   GLuint CurrentPosition;
#endif
};


/**
 * Context state for fragment programs.
 */
struct gl_fragment_program_state
{
   GLboolean Enabled;     /**< User-set fragment program enable flag */
   GLboolean _Enabled;    /**< Fragment program enabled and valid? */
   GLboolean _Active;
   struct gl_fragment_program *Current;  /**< User-bound fragment program */

   /** Currently enabled and valid program (including internal programs
    * and compiled shader programs).
    */
   struct gl_fragment_program *_Current;

   GLfloat Parameters[MAX_PROGRAM_ENV_PARAMS][4]; /**< Env params */

   /** Should fixed-function texturing be implemented with a fragment prog? */
   GLboolean _MaintainTexEnvProgram;
   GLboolean _UseTexEnvProgram;

   /** Program to emulate fixed-function texture env/combine (see above) */
   struct gl_fragment_program *_TexEnvProgram;

#if FEATURE_MESA_program_debug
   GLprogramcallbackMESA Callback;
   GLvoid *CallbackData;
   GLboolean CallbackEnabled;
   GLuint CurrentPosition;
#endif
};


/**
 * ATI_fragment_shader runtime state
 */
#define ATI_FS_INPUT_PRIMARY 0
#define ATI_FS_INPUT_SECONDARY 1

struct atifs_instruction;
struct atifs_setupinst;

/**
 * ATI fragment shader
 */
struct ati_fragment_shader
{
   GLuint Id;
   GLint RefCount;
   struct atifs_instruction *Instructions[2];
   struct atifs_setupinst *SetupInst[2];
   GLfloat Constants[8][4];
   GLbitfield LocalConstDef;  /** Indicates which constants have been set */
   GLubyte numArithInstr[2];
   GLubyte regsAssigned[2];
   GLubyte NumPasses;         /** 1 or 2 */
   GLubyte cur_pass;
   GLubyte last_optype;
   GLboolean interpinp1;
   GLboolean isValid;
   GLuint swizzlerq;
};

/**
 * Context state for GL_ATI_fragment_shader
 */
struct gl_ati_fragment_shader_state
{
   GLboolean Enabled;
   GLboolean _Enabled;                      /** enabled and valid shader? */
   GLboolean Compiling;
   GLfloat GlobalConstants[8][4];
   struct ati_fragment_shader *Current;
};


/**
 * Occlusion/timer query object.
 */
struct gl_query_object
{
   GLuint Id;
   GLuint64EXT Result; /* the counter */
   GLboolean Active;   /* inside Begin/EndQuery */
   GLboolean Ready;    /* result is ready */
};


/**
 * Context state for query objects.
 */
struct gl_query_state
{
   struct _mesa_HashTable *QueryObjects;
   struct gl_query_object *CurrentOcclusionObject; /* GL_ARB_occlusion_query */
   struct gl_query_object *CurrentTimerObject;     /* GL_EXT_timer_query */
};



/**
 * A GLSL vertex or fragment shader object.
 */
struct gl_shader
{
   GLenum Type;  /**< GL_FRAGMENT_SHADER || GL_VERTEX_SHADER (first field!) */
   GLuint Name;  /**< AKA the handle */
   GLint RefCount;  /**< Reference count */
   GLboolean DeletePending;

   const GLchar *Source;  /**< Source code string */
   GLboolean CompileStatus;
   struct gl_program *Program;  /**< Post-compile assembly code */
   GLchar *InfoLog;
   GLboolean Main;  /**< shader defines main() */
};


/**
 * A GLSL program object.
 * Basically a linked collection of vertex and fragment shaders.
 */
struct gl_shader_program
{
   GLenum Type;  /**< Always GL_SHADER_PROGRAM (internal token) */
   GLuint Name;  /**< aka handle or ID */
   GLint RefCount;  /**< Reference count */
   GLboolean DeletePending;

   GLuint NumShaders;          /**< number of attached shaders */
   struct gl_shader **Shaders; /**< List of attached the shaders */

   /** User-defined attribute bindings (glBindAttribLocation) */
   struct gl_program_parameter_list *Attributes;

   /* post-link info: */
   struct gl_vertex_program *VertexProgram;     /**< Linked vertex program */
   struct gl_fragment_program *FragmentProgram; /**< Linked fragment prog */
   struct gl_uniform_list *Uniforms;
   struct gl_program_parameter_list *Varying;
   GLboolean LinkStatus;   /**< GL_LINK_STATUS */
   GLboolean Validated;
   GLchar *InfoLog;
};   


/**
 * Context state for GLSL vertex/fragment shaders.
 */
struct gl_shader_state
{
   struct gl_shader_program *CurrentProgram; /**< The user-bound program */
   /** Driver-selectable options: */
   GLboolean EmitHighLevelInstructions; /**< IF/ELSE/ENDIF vs. BRA, etc. */
   GLboolean EmitCondCodes;             /**< Use condition codes? */
   GLboolean EmitComments;              /**< Annotated instructions */
   void *MemPool;
};


/**
 * State which can be shared by multiple contexts:
 */
struct gl_shared_state
{
   _glthread_Mutex Mutex;		   /**< for thread safety */
   GLint RefCount;			   /**< Reference count */
   struct _mesa_HashTable *DisplayList;	   /**< Display lists hash table */
   struct _mesa_HashTable *TexObjects;	   /**< Texture objects hash table */

   /**
    * \name Default texture objects (shared by all multi-texture units)
    */
   /*@{*/
   struct gl_texture_object *Default1D;
   struct gl_texture_object *Default2D;
   struct gl_texture_object *Default3D;
   struct gl_texture_object *DefaultCubeMap;
   struct gl_texture_object *DefaultRect;
   struct gl_texture_object *Default1DArray;
   struct gl_texture_object *Default2DArray;
   /*@}*/

   /**
    * \name Thread safety and statechange notification for texture
    * objects. 
    *
    * \todo Improve the granularity of locking.
    */
   /*@{*/
   _glthread_Mutex TexMutex;		/**< texobj thread safety */
   GLuint TextureStateStamp;	        /**< state notification for shared tex */
   /*@}*/


   /**
    * \name Vertex/fragment programs
    */
   /*@{*/
   struct _mesa_HashTable *Programs; /**< All vertex/fragment programs */
#if FEATURE_ARB_vertex_program
   struct gl_vertex_program *DefaultVertexProgram;
#endif
#if FEATURE_ARB_fragment_program
   struct gl_fragment_program *DefaultFragmentProgram;
#endif
   /*@}*/

#if FEATURE_ATI_fragment_shader
   struct _mesa_HashTable *ATIShaders;
   struct ati_fragment_shader *DefaultFragmentShader;
#endif

#if FEATURE_ARB_vertex_buffer_object || FEATURE_ARB_pixel_buffer_object
   struct _mesa_HashTable *BufferObjects;
#endif

#if FEATURE_ARB_shader_objects
   /** Table of both gl_shader and gl_shader_program objects */
   struct _mesa_HashTable *ShaderObjects;
#endif

#if FEATURE_EXT_framebuffer_object
   struct _mesa_HashTable *RenderBuffers;
   struct _mesa_HashTable *FrameBuffers;
#endif

   /** Objects associated with the GL_APPLE_vertex_array_object extension. */
   struct _mesa_HashTable *ArrayObjects;

   void *DriverData;  /**< Device driver shared state */
};




/**
 * A renderbuffer stores colors or depth values or stencil values.
 * A framebuffer object will have a collection of these.
 * Data are read/written to the buffer with a handful of Get/Put functions.
 *
 * Instances of this object are allocated with the Driver's NewRenderbuffer
 * hook.  Drivers will likely wrap this class inside a driver-specific
 * class to simulate inheritance.
 */
struct gl_renderbuffer
{
#define RB_MAGIC 0xaabbccdd
   int Magic; /** XXX TEMPORARY DEBUG INFO */
   _glthread_Mutex Mutex;		   /**< for thread safety */
   GLuint ClassID;        /**< Useful for drivers */
   GLuint Name;
   GLint RefCount;
   GLuint Width, Height;
   GLenum InternalFormat; /**< The user-specified format */
   GLenum _ActualFormat;  /**< The driver-chosen format */
   GLenum _BaseFormat;    /**< Either GL_RGB, GL_RGBA, GL_DEPTH_COMPONENT or
                               GL_STENCIL_INDEX. */
   GLenum DataType;      /**< Type of values passed to the Get/Put functions */
   GLubyte RedBits;      /**< Bits of red per pixel */
   GLubyte GreenBits;
   GLubyte BlueBits;
   GLubyte AlphaBits;
   GLubyte IndexBits;
   GLubyte DepthBits;
   GLubyte StencilBits;
   GLvoid *Data;        /**< This may not be used by some kinds of RBs */

   /* Used to wrap one renderbuffer around another: */
   struct gl_renderbuffer *Wrapped;

   /* Delete this renderbuffer */
   void (*Delete)(struct gl_renderbuffer *rb);

   /* Allocate new storage for this renderbuffer */
   GLboolean (*AllocStorage)(GLcontext *ctx, struct gl_renderbuffer *rb,
                             GLenum internalFormat,
                             GLuint width, GLuint height);

   /* Lock/Unlock are called before/after calling the Get/Put functions.
    * Not sure this is the right place for these yet.
   void (*Lock)(GLcontext *ctx, struct gl_renderbuffer *rb);
   void (*Unlock)(GLcontext *ctx, struct gl_renderbuffer *rb);
    */

   /* Return a pointer to the element/pixel at (x,y).
    * Should return NULL if the buffer memory can't be directly addressed.
    */
   void *(*GetPointer)(GLcontext *ctx, struct gl_renderbuffer *rb,
                       GLint x, GLint y);

   /* Get/Read a row of values.
    * The values will be of format _BaseFormat and type DataType.
    */
   void (*GetRow)(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  GLint x, GLint y, void *values);

   /* Get/Read values at arbitrary locations.
    * The values will be of format _BaseFormat and type DataType.
    */
   void (*GetValues)(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                     const GLint x[], const GLint y[], void *values);

   /* Put/Write a row of values.
    * The values will be of format _BaseFormat and type DataType.
    */
   void (*PutRow)(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                  GLint x, GLint y, const void *values, const GLubyte *mask);

   /* Put/Write a row of RGB values.  This is a special-case routine that's
    * only used for RGBA renderbuffers when the source data is GL_RGB. That's
    * a common case for glDrawPixels and some triangle routines.
    * The values will be of format GL_RGB and type DataType.
    */
   void (*PutRowRGB)(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                    GLint x, GLint y, const void *values, const GLubyte *mask);


   /* Put/Write a row of identical values.
    * The values will be of format _BaseFormat and type DataType.
    */
   void (*PutMonoRow)(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                     GLint x, GLint y, const void *value, const GLubyte *mask);

   /* Put/Write values at arbitrary locations.
    * The values will be of format _BaseFormat and type DataType.
    */
   void (*PutValues)(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                     const GLint x[], const GLint y[], const void *values,
                     const GLubyte *mask);
   /* Put/Write identical values at arbitrary locations.
    * The values will be of format _BaseFormat and type DataType.
    */
   void (*PutMonoValues)(GLcontext *ctx, struct gl_renderbuffer *rb,
                         GLuint count, const GLint x[], const GLint y[],
                         const void *value, const GLubyte *mask);
};


/**
 * A renderbuffer attachment point points to either a texture object
 * (and specifies a mipmap level, cube face or 3D texture slice) or
 * points to a renderbuffer.
 */
struct gl_renderbuffer_attachment
{
   GLenum Type;  /**< \c GL_NONE or \c GL_TEXTURE or \c GL_RENDERBUFFER_EXT */
   GLboolean Complete;

   /**
    * If \c Type is \c GL_RENDERBUFFER_EXT, this stores a pointer to the
    * application supplied renderbuffer object.
    */
   struct gl_renderbuffer *Renderbuffer;

   /**
    * If \c Type is \c GL_TEXTURE, this stores a pointer to the application
    * supplied texture object.
    */
   struct gl_texture_object *Texture;
   GLuint TextureLevel; /**< Attached mipmap level. */
   GLuint CubeMapFace;  /**< 0 .. 5, for cube map textures. */
   GLuint Zoffset;      /**< Slice for 3D textures,  or layer for both 1D
                         * and 2D array textures */
};


/**
 * A framebuffer is a collection of renderbuffers (color, depth, stencil, etc).
 * In C++ terms, think of this as a base class from which device drivers
 * will make derived classes.
 */
struct gl_framebuffer
{
   _glthread_Mutex Mutex;		   /**< for thread safety */
   GLuint Name;      /* if zero, this is a window system framebuffer */
   GLint RefCount;
   GLboolean DeletePending;

   GLvisual Visual;	/**< The framebuffer's visual.
                             Immutable if this is a window system buffer.
                             Computed from attachments if user-made FBO. */

   GLboolean Initialized;

   GLuint Width, Height;	/**< size of frame buffer in pixels */

   /** \name  Drawing bounds (Intersection of buffer size and scissor box) */
   /*@{*/
   GLint _Xmin, _Xmax;  /**< inclusive */
   GLint _Ymin, _Ymax;  /**< exclusive */
   /*@}*/

   /** \name  Derived Z buffer stuff */
   /*@{*/
   GLuint _DepthMax;	/**< Max depth buffer value */
   GLfloat _DepthMaxF;	/**< Float max depth buffer value */
   GLfloat _MRD;	/**< minimum resolvable difference in Z values */
   /*@}*/

   GLenum _Status; /* One of the GL_FRAMEBUFFER_(IN)COMPLETE_* tokens */

   /* Array of all renderbuffer attachments, indexed by BUFFER_* tokens. */
   struct gl_renderbuffer_attachment Attachment[BUFFER_COUNT];

   /* In unextended OpenGL these vars are part of the GL_COLOR_BUFFER
    * attribute group and GL_PIXEL attribute group, respectively.
    */
   GLenum ColorDrawBuffer[MAX_DRAW_BUFFERS];
   GLenum ColorReadBuffer;

   /** Computed from ColorDraw/ReadBuffer above */
   GLuint _NumColorDrawBuffers;
   GLint _ColorDrawBufferIndexes[MAX_DRAW_BUFFERS]; /**< BUFFER_x or -1 */
   GLint _ColorReadBufferIndex; /* -1 = None */
   struct gl_renderbuffer *_ColorDrawBuffers[MAX_DRAW_BUFFERS];
   struct gl_renderbuffer *_ColorReadBuffer;

   /** The Actual depth/stencil buffers to use.  May be wrappers around the
    * depth/stencil buffers attached above. */
   struct gl_renderbuffer *_DepthBuffer;
   struct gl_renderbuffer *_StencilBuffer;

   /** Delete this framebuffer */
   void (*Delete)(struct gl_framebuffer *fb);
};


/**
 * Limits for vertex and fragment programs.
 */
struct gl_program_constants
{
   /* logical limits */
   GLuint MaxInstructions;
   GLuint MaxAluInstructions; /* fragment programs only, for now */
   GLuint MaxTexInstructions; /* fragment programs only, for now */
   GLuint MaxTexIndirections; /* fragment programs only, for now */
   GLuint MaxAttribs;
   GLuint MaxTemps;
   GLuint MaxAddressRegs; /* vertex program only, for now */
   GLuint MaxParameters;
   GLuint MaxLocalParams;
   GLuint MaxEnvParams;
   /* native/hardware limits */
   GLuint MaxNativeInstructions;
   GLuint MaxNativeAluInstructions; /* fragment programs only, for now */
   GLuint MaxNativeTexInstructions; /* fragment programs only, for now */
   GLuint MaxNativeTexIndirections; /* fragment programs only, for now */
   GLuint MaxNativeAttribs;
   GLuint MaxNativeTemps;
   GLuint MaxNativeAddressRegs; /* vertex program only, for now */
   GLuint MaxNativeParameters;
   /* For shaders */
   GLuint MaxUniformComponents;
};


/**
 * Constants which may be overridden by device driver during context creation
 * but are never changed after that.
 */
struct gl_constants
{
   GLint MaxTextureLevels;		/**< Maximum number of allowed mipmap levels. */ 
   GLint Max3DTextureLevels;		/**< Maximum number of allowed mipmap levels for 3D texture targets. */
   GLint MaxCubeTextureLevels;          /**< Maximum number of allowed mipmap levels for GL_ARB_texture_cube_map */
   GLint MaxArrayTextureLayers;         /**< Maximum number of layers in an array texture. */
   GLint MaxTextureRectSize;            /* GL_NV_texture_rectangle */
   GLuint MaxTextureCoordUnits;
   GLuint MaxTextureImageUnits;
   GLuint MaxTextureUnits;              /* = MIN(CoordUnits, ImageUnits) */
   GLfloat MaxTextureMaxAnisotropy;	/* GL_EXT_texture_filter_anisotropic */
   GLfloat MaxTextureLodBias;           /* GL_EXT_texture_lod_bias */
   GLuint MaxArrayLockSize;
   GLint SubPixelBits;
   GLfloat MinPointSize, MaxPointSize;		/* aliased */
   GLfloat MinPointSizeAA, MaxPointSizeAA;	/* antialiased */
   GLfloat PointSizeGranularity;
   GLfloat MinLineWidth, MaxLineWidth;		/* aliased */
   GLfloat MinLineWidthAA, MaxLineWidthAA;	/* antialiased */
   GLfloat LineWidthGranularity;
   GLuint MaxColorTableSize;
   GLuint MaxConvolutionWidth;
   GLuint MaxConvolutionHeight;
   GLuint MaxClipPlanes;
   GLuint MaxLights;
   GLfloat MaxShininess;			/* GL_NV_light_max_exponent */
   GLfloat MaxSpotExponent;			/* GL_NV_light_max_exponent */
   GLuint MaxViewportWidth, MaxViewportHeight;
   struct gl_program_constants VertexProgram;    /* GL_ARB_vertex_program */
   struct gl_program_constants FragmentProgram;  /* GL_ARB_fragment_program */
   /* shared by vertex and fragment program: */
   GLuint MaxProgramMatrices;
   GLuint MaxProgramMatrixStackDepth;
   /* vertex array / buffer object bounds checking */
   GLboolean CheckArrayBounds;
   /* GL_ARB_draw_buffers */
   GLuint MaxDrawBuffers;
   /* GL_OES_read_format */
   GLenum ColorReadFormat;
   GLenum ColorReadType;
   /* GL_EXT_framebuffer_object */
   GLuint MaxColorAttachments;
   GLuint MaxRenderbufferSize;
   /* GL_ARB_vertex_shader */
   GLuint MaxVertexTextureImageUnits;
   GLuint MaxVarying;
};


/**
 * Enable flag for each OpenGL extension.  Different device drivers will
 * enable different extensions at runtime.
 */
struct gl_extensions
{
   /**
    * \name Flags to quickly test if certain extensions are available.
    * 
    * Not every extension needs to have such a flag, but it's encouraged.
    */
   /*@{*/
   GLboolean dummy;  /* don't remove this! */
   GLboolean ARB_depth_texture;
   GLboolean ARB_draw_buffers;
   GLboolean ARB_fragment_program;
   GLboolean ARB_fragment_program_shadow;
   GLboolean ARB_fragment_shader;
   GLboolean ARB_half_float_pixel;
   GLboolean ARB_imaging;
   GLboolean ARB_multisample;
   GLboolean ARB_multitexture;
   GLboolean ARB_occlusion_query;
   GLboolean ARB_point_sprite;
   GLboolean ARB_shader_objects;
   GLboolean ARB_shading_language_100;
   GLboolean ARB_shading_language_120;
   GLboolean ARB_shadow;
   GLboolean ARB_texture_border_clamp;
   GLboolean ARB_texture_compression;
   GLboolean ARB_texture_cube_map;
   GLboolean ARB_texture_env_combine;
   GLboolean ARB_texture_env_crossbar;
   GLboolean ARB_texture_env_dot3;
   GLboolean ARB_texture_float;
   GLboolean ARB_texture_mirrored_repeat;
   GLboolean ARB_texture_non_power_of_two;
   GLboolean ARB_transpose_matrix;
   GLboolean ARB_vertex_buffer_object;
   GLboolean ARB_vertex_program;
   GLboolean ARB_vertex_shader;
   GLboolean ARB_window_pos;
   GLboolean EXT_abgr;
   GLboolean EXT_bgra;
   GLboolean EXT_blend_color;
   GLboolean EXT_blend_equation_separate;
   GLboolean EXT_blend_func_separate;
   GLboolean EXT_blend_logic_op;
   GLboolean EXT_blend_minmax;
   GLboolean EXT_blend_subtract;
   GLboolean EXT_clip_volume_hint;
   GLboolean EXT_cull_vertex;
   GLboolean EXT_convolution;
   GLboolean EXT_compiled_vertex_array;
   GLboolean EXT_copy_texture;
   GLboolean EXT_depth_bounds_test;
   GLboolean EXT_draw_range_elements;
   GLboolean EXT_framebuffer_object;
   GLboolean EXT_fog_coord;
   GLboolean EXT_framebuffer_blit;
   GLboolean EXT_gpu_program_parameters;
   GLboolean EXT_histogram;
   GLboolean EXT_multi_draw_arrays;
   GLboolean EXT_paletted_texture;
   GLboolean EXT_packed_depth_stencil;
   GLboolean EXT_packed_pixels;
   GLboolean EXT_pixel_buffer_object;
   GLboolean EXT_point_parameters;
   GLboolean EXT_polygon_offset;
   GLboolean EXT_rescale_normal;
   GLboolean EXT_shadow_funcs;
   GLboolean EXT_secondary_color;
   GLboolean EXT_separate_specular_color;
   GLboolean EXT_shared_texture_palette;
   GLboolean EXT_stencil_wrap;
   GLboolean EXT_stencil_two_side;
   GLboolean EXT_subtexture;
   GLboolean EXT_texture;
   GLboolean EXT_texture_object;
   GLboolean EXT_texture3D;
   GLboolean EXT_texture_compression_s3tc;
   GLboolean EXT_texture_env_add;
   GLboolean EXT_texture_env_combine;
   GLboolean EXT_texture_env_dot3;
   GLboolean EXT_texture_filter_anisotropic;
   GLboolean EXT_texture_lod_bias;
   GLboolean EXT_texture_mirror_clamp;
   GLboolean EXT_texture_sRGB;
   GLboolean EXT_timer_query;
   GLboolean EXT_vertex_array;
   GLboolean EXT_vertex_array_set;
   /* vendor extensions */
   GLboolean APPLE_client_storage;
   GLboolean APPLE_packed_pixels;
   GLboolean APPLE_vertex_array_object;
   GLboolean ATI_texture_mirror_once;
   GLboolean ATI_texture_env_combine3;
   GLboolean ATI_fragment_shader;
   GLboolean ATI_separate_stencil;
   GLboolean IBM_rasterpos_clip;
   GLboolean IBM_multimode_draw_arrays;
   GLboolean MESA_pack_invert;
   GLboolean MESA_packed_depth_stencil;
   GLboolean MESA_program_debug;
   GLboolean MESA_resize_buffers;
   GLboolean MESA_ycbcr_texture;
   GLboolean MESA_texture_array;
   GLboolean NV_blend_square;
   GLboolean NV_fragment_program;
   GLboolean NV_light_max_exponent;
   GLboolean NV_point_sprite;
   GLboolean NV_texgen_reflection;
   GLboolean NV_texture_rectangle;
   GLboolean NV_vertex_program;
   GLboolean NV_vertex_program1_1;
   GLboolean OES_read_format;
   GLboolean SGI_color_matrix;
   GLboolean SGI_color_table;
   GLboolean SGI_texture_color_table;
   GLboolean SGIS_generate_mipmap;
   GLboolean SGIS_texture_edge_clamp;
   GLboolean SGIS_texture_lod;
   GLboolean SGIX_depth_texture;
   GLboolean SGIX_shadow;
   GLboolean SGIX_shadow_ambient; /* or GL_ARB_shadow_ambient */
   GLboolean TDFX_texture_compression_FXT1;
   GLboolean S3_s3tc;
   /*@}*/
   /* The extension string */
   const GLubyte *String;
};


/**
 * A stack of matrices (projection, modelview, color, texture, etc).
 */
struct gl_matrix_stack
{
   GLmatrix *Top;      /**< points into Stack */
   GLmatrix *Stack;    /**< array [MaxDepth] of GLmatrix */
   GLuint Depth;       /**< 0 <= Depth < MaxDepth */
   GLuint MaxDepth;    /**< size of Stack[] array */
   GLuint DirtyFlag;   /**< _NEW_MODELVIEW or _NEW_PROJECTION, for example */
};


/**
 * \name Bits for image transfer operations 
 *
 * \sa __GLcontextRec::ImageTransferState.
 */
/*@{*/
#define IMAGE_SCALE_BIAS_BIT                      0x1
#define IMAGE_SHIFT_OFFSET_BIT                    0x2
#define IMAGE_MAP_COLOR_BIT                       0x4
#define IMAGE_COLOR_TABLE_BIT                     0x8
#define IMAGE_CONVOLUTION_BIT                     0x10
#define IMAGE_POST_CONVOLUTION_SCALE_BIAS         0x20
#define IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT    0x40
#define IMAGE_COLOR_MATRIX_BIT                    0x80
#define IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT   0x100
#define IMAGE_HISTOGRAM_BIT                       0x200
#define IMAGE_MIN_MAX_BIT                         0x400
#define IMAGE_CLAMP_BIT                           0x800 /* extra */


/** Pixel Transfer ops up to convolution */
#define IMAGE_PRE_CONVOLUTION_BITS (IMAGE_SCALE_BIAS_BIT |     \
                                    IMAGE_SHIFT_OFFSET_BIT |   \
                                    IMAGE_MAP_COLOR_BIT |      \
                                    IMAGE_COLOR_TABLE_BIT)

/** Pixel transfer ops after convolution */
#define IMAGE_POST_CONVOLUTION_BITS (IMAGE_POST_CONVOLUTION_SCALE_BIAS |      \
                                     IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT | \
                                     IMAGE_COLOR_MATRIX_BIT |                 \
                                     IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT |\
                                     IMAGE_HISTOGRAM_BIT |                    \
                                     IMAGE_MIN_MAX_BIT)
/*@}*/


/**
 * \name Bits to indicate what state has changed.  
 *
 * 4 unused flags.
 */
/*@{*/
#define _NEW_MODELVIEW		0x1        /**< __GLcontextRec::ModelView */
#define _NEW_PROJECTION		0x2        /**< __GLcontextRec::Projection */
#define _NEW_TEXTURE_MATRIX	0x4        /**< __GLcontextRec::TextureMatrix */
#define _NEW_COLOR_MATRIX	0x8        /**< __GLcontextRec::ColorMatrix */
#define _NEW_ACCUM		0x10       /**< __GLcontextRec::Accum */
#define _NEW_COLOR		0x20       /**< __GLcontextRec::Color */
#define _NEW_DEPTH		0x40       /**< __GLcontextRec::Depth */
#define _NEW_EVAL		0x80       /**< __GLcontextRec::Eval, __GLcontextRec::EvalMap */
#define _NEW_FOG		0x100      /**< __GLcontextRec::Fog */
#define _NEW_HINT		0x200      /**< __GLcontextRec::Hint */
#define _NEW_LIGHT		0x400      /**< __GLcontextRec::Light */
#define _NEW_LINE		0x800      /**< __GLcontextRec::Line */
#define _NEW_PIXEL		0x1000     /**< __GLcontextRec::Pixel */
#define _NEW_POINT		0x2000     /**< __GLcontextRec::Point */
#define _NEW_POLYGON		0x4000     /**< __GLcontextRec::Polygon */
#define _NEW_POLYGONSTIPPLE	0x8000     /**< __GLcontextRec::PolygonStipple */
#define _NEW_SCISSOR		0x10000    /**< __GLcontextRec::Scissor */
#define _NEW_STENCIL		0x20000    /**< __GLcontextRec::Stencil */
#define _NEW_TEXTURE		0x40000    /**< __GLcontextRec::Texture */
#define _NEW_TRANSFORM		0x80000    /**< __GLcontextRec::Transform */
#define _NEW_VIEWPORT		0x100000   /**< __GLcontextRec::Viewport */
#define _NEW_PACKUNPACK		0x200000   /**< __GLcontextRec::Pack, __GLcontextRec::Unpack */
#define _NEW_ARRAY	        0x400000   /**< __GLcontextRec::Array */
#define _NEW_RENDERMODE		0x800000   /**< __GLcontextRec::RenderMode, __GLcontextRec::Feedback, __GLcontextRec::Select */
#define _NEW_BUFFERS            0x1000000  /**< __GLcontextRec::Visual, __GLcontextRec::DrawBuffer, */
#define _NEW_MULTISAMPLE        0x2000000  /**< __GLcontextRec::Multisample */
#define _NEW_TRACK_MATRIX       0x4000000  /**< __GLcontextRec::VertexProgram */
#define _NEW_PROGRAM            0x8000000  /**< __GLcontextRec::VertexProgram */
#define _NEW_ALL ~0
/*@}*/


/**
 * \name Bits to track array state changes 
 *
 * Also used to summarize array enabled.
 */
/*@{*/
#define _NEW_ARRAY_VERTEX           VERT_BIT_POS
#define _NEW_ARRAY_WEIGHT           VERT_BIT_WEIGHT
#define _NEW_ARRAY_NORMAL           VERT_BIT_NORMAL
#define _NEW_ARRAY_COLOR0           VERT_BIT_COLOR0
#define _NEW_ARRAY_COLOR1           VERT_BIT_COLOR1
#define _NEW_ARRAY_FOGCOORD         VERT_BIT_FOG
#define _NEW_ARRAY_INDEX            VERT_BIT_COLOR_INDEX
#define _NEW_ARRAY_EDGEFLAG         VERT_BIT_EDGEFLAG
#define _NEW_ARRAY_TEXCOORD_0       VERT_BIT_TEX0
#define _NEW_ARRAY_TEXCOORD_1       VERT_BIT_TEX1
#define _NEW_ARRAY_TEXCOORD_2       VERT_BIT_TEX2
#define _NEW_ARRAY_TEXCOORD_3       VERT_BIT_TEX3
#define _NEW_ARRAY_TEXCOORD_4       VERT_BIT_TEX4
#define _NEW_ARRAY_TEXCOORD_5       VERT_BIT_TEX5
#define _NEW_ARRAY_TEXCOORD_6       VERT_BIT_TEX6
#define _NEW_ARRAY_TEXCOORD_7       VERT_BIT_TEX7
#define _NEW_ARRAY_ATTRIB_0         VERT_BIT_GENERIC0  /* start at bit 16 */
#define _NEW_ARRAY_ALL              0xffffffff


#define _NEW_ARRAY_TEXCOORD(i) (_NEW_ARRAY_TEXCOORD_0 << (i))
#define _NEW_ARRAY_ATTRIB(i) (_NEW_ARRAY_ATTRIB_0 << (i))
/*@}*/


/**
 * \name A bunch of flags that we think might be useful to drivers.
 * 
 * Set in the __GLcontextRec::_TriangleCaps bitfield.
 */
/*@{*/
#define DD_FLATSHADE                0x1
#define DD_SEPARATE_SPECULAR        0x2
#define DD_TRI_CULL_FRONT_BACK      0x4 /* special case on some hw */
#define DD_TRI_LIGHT_TWOSIDE        0x8
#define DD_TRI_UNFILLED             0x10
#define DD_TRI_SMOOTH               0x20
#define DD_TRI_STIPPLE              0x40
#define DD_TRI_OFFSET               0x80
#define DD_LINE_SMOOTH              0x100
#define DD_LINE_STIPPLE             0x200
#define DD_LINE_WIDTH               0x400
#define DD_POINT_SMOOTH             0x800
#define DD_POINT_SIZE               0x1000
#define DD_POINT_ATTEN              0x2000
#define DD_TRI_TWOSTENCIL           0x4000
/*@}*/


/**
 * \name Define the state changes under which each of these bits might change
 */
/*@{*/
#define _DD_NEW_FLATSHADE                _NEW_LIGHT
#define _DD_NEW_SEPARATE_SPECULAR        (_NEW_LIGHT | _NEW_FOG | _NEW_PROGRAM)
#define _DD_NEW_TRI_CULL_FRONT_BACK      _NEW_POLYGON
#define _DD_NEW_TRI_LIGHT_TWOSIDE        _NEW_LIGHT
#define _DD_NEW_TRI_UNFILLED             _NEW_POLYGON
#define _DD_NEW_TRI_SMOOTH               _NEW_POLYGON
#define _DD_NEW_TRI_STIPPLE              _NEW_POLYGON
#define _DD_NEW_TRI_OFFSET               _NEW_POLYGON
#define _DD_NEW_LINE_SMOOTH              _NEW_LINE
#define _DD_NEW_LINE_STIPPLE             _NEW_LINE
#define _DD_NEW_LINE_WIDTH               _NEW_LINE
#define _DD_NEW_POINT_SMOOTH             _NEW_POINT
#define _DD_NEW_POINT_SIZE               _NEW_POINT
#define _DD_NEW_POINT_ATTEN              _NEW_POINT
/*@}*/


#define _MESA_NEW_NEED_EYE_COORDS         (_NEW_LIGHT |		\
                                           _NEW_TEXTURE |	\
                                           _NEW_POINT |		\
                                           _NEW_PROGRAM |	\
                                           _NEW_MODELVIEW)

#define _MESA_NEW_NEED_NORMALS            (_NEW_LIGHT |		\
                                           _NEW_TEXTURE)

#define _IMAGE_NEW_TRANSFER_STATE         (_NEW_PIXEL | _NEW_COLOR_MATRIX)




/*
 * Forward declaration of display list data types:
 */
union node;
typedef union node Node;


/* This has to be included here. */
#include "dd.h"


#define NUM_VERTEX_FORMAT_ENTRIES (sizeof(GLvertexformat) / sizeof(void *))

/**
 * Core Mesa's support for tnl modules:
 */
struct gl_tnl_module
{
   /**
    * Vertex format to be lazily swapped into current dispatch.
    */
   const GLvertexformat *Current;

   /**
    * \name Record of functions swapped out.  
    * On restore, only need to swap these functions back in.
    */
   /*@{*/
   struct {
       _glapi_proc * location;
       _glapi_proc function;
   } Swapped[NUM_VERTEX_FORMAT_ENTRIES];
   GLuint SwapCount;
   /*@}*/
};

/* Strictly this is a tnl/ private concept, but it doesn't seem
 * worthwhile adding a tnl private structure just to hold this one bit
 * of information:
 */
#define MESA_DLIST_DANGLING_REFS     0x1 

/* Provide a location where information about a display list can be
 * collected.  Could be extended with driverPrivate structures,
 * etc. in the future.
 */
struct mesa_display_list
{
   Node *node;
   GLuint id;
   GLbitfield flags;
};


/**
 * State used during display list compilation and execution.
 */
struct gl_dlist_state
{
   GLuint CallDepth;		/**< Current recursion calling depth */

   struct mesa_display_list *CurrentList;
   Node *CurrentListPtr;	/**< Head of list being compiled */
   GLuint CurrentListNum;	/**< Number of the list being compiled */
   Node *CurrentBlock;		/**< Pointer to current block of nodes */
   GLuint CurrentPos;		/**< Index into current block of nodes */

   GLvertexformat ListVtxfmt;

   GLubyte ActiveAttribSize[VERT_ATTRIB_MAX];
   GLfloat CurrentAttrib[VERT_ATTRIB_MAX][4];
   
   GLubyte ActiveMaterialSize[MAT_ATTRIB_MAX];
   GLfloat CurrentMaterial[MAT_ATTRIB_MAX][4];

   GLubyte ActiveIndex;
   GLfloat CurrentIndex;
   
   GLubyte ActiveEdgeFlag;
   GLboolean CurrentEdgeFlag;
};


/**
 * Mesa rendering context.
 *
 * This is the central context data structure for Mesa.  Almost all
 * OpenGL state is contained in this structure.
 * Think of this as a base class from which device drivers will derive
 * sub classes.
 *
 * The GLcontext typedef names this structure.
 */
struct __GLcontextRec
{
   /** State possibly shared with other contexts in the address space */
   struct gl_shared_state *Shared;

   /** \name API function pointer tables */
   /*@{*/
   struct _glapi_table *Save;	/**< Display list save functions */
   struct _glapi_table *Exec;	/**< Execute functions */
   struct _glapi_table *CurrentDispatch;  /**< == Save or Exec !! */
   /*@}*/

   GLvisual Visual;
   GLframebuffer *DrawBuffer;	/**< buffer for writing */
   GLframebuffer *ReadBuffer;	/**< buffer for reading */
   GLframebuffer *WinSysDrawBuffer;  /**< set with MakeCurrent */
   GLframebuffer *WinSysReadBuffer;  /**< set with MakeCurrent */

   /**
    * Device driver function pointer table
    */
   struct dd_function_table Driver;

   void *DriverCtx;	/**< Points to device driver context/state */

   /** Core/Driver constants */
   struct gl_constants Const;

   /** \name The various 4x4 matrix stacks */
   /*@{*/
   struct gl_matrix_stack ModelviewMatrixStack;
   struct gl_matrix_stack ProjectionMatrixStack;
   struct gl_matrix_stack ColorMatrixStack;
   struct gl_matrix_stack TextureMatrixStack[MAX_TEXTURE_COORD_UNITS];
   struct gl_matrix_stack ProgramMatrixStack[MAX_PROGRAM_MATRICES];
   struct gl_matrix_stack *CurrentStack; /**< Points to one of the above stacks */
   /*@}*/

   /** Combined modelview and projection matrix */
   GLmatrix _ModelProjectMatrix;

   /** \name Display lists */
   struct gl_dlist_state ListState;

   GLboolean ExecuteFlag;	/**< Execute GL commands? */
   GLboolean CompileFlag;	/**< Compile GL commands into display list? */

   /** Extension information */
   struct gl_extensions Extensions;

   /** \name State attribute stack (for glPush/PopAttrib) */
   /*@{*/
   GLuint AttribStackDepth;
   struct gl_attrib_node *AttribStack[MAX_ATTRIB_STACK_DEPTH];
   /*@}*/

   /** \name Renderer attribute groups
    * 
    * We define a struct for each attribute group to make pushing and popping
    * attributes easy.  Also it's a good organization.
    */
   /*@{*/
   struct gl_accum_attrib	Accum;		/**< Accum buffer attributes */
   struct gl_colorbuffer_attrib	Color;		/**< Color buffer attributes */
   struct gl_current_attrib	Current;	/**< Current attributes */
   struct gl_depthbuffer_attrib	Depth;		/**< Depth buffer attributes */
   struct gl_eval_attrib	Eval;		/**< Eval attributes */
   struct gl_fog_attrib		Fog;		/**< Fog attributes */
   struct gl_hint_attrib	Hint;		/**< Hint attributes */
   struct gl_light_attrib	Light;		/**< Light attributes */
   struct gl_line_attrib	Line;		/**< Line attributes */
   struct gl_list_attrib	List;		/**< List attributes */
   struct gl_multisample_attrib Multisample;
   struct gl_pixel_attrib	Pixel;		/**< Pixel attributes */
   struct gl_point_attrib	Point;		/**< Point attributes */
   struct gl_polygon_attrib	Polygon;	/**< Polygon attributes */
   GLuint PolygonStipple[32];			/**< Polygon stipple */
   struct gl_scissor_attrib	Scissor;	/**< Scissor attributes */
   struct gl_stencil_attrib	Stencil;	/**< Stencil buffer attributes */
   struct gl_texture_attrib	Texture;	/**< Texture attributes */
   struct gl_transform_attrib	Transform;	/**< Transformation attributes */
   struct gl_viewport_attrib	Viewport;	/**< Viewport attributes */
   /*@}*/

   /** \name Client attribute stack */
   /*@{*/
   GLuint ClientAttribStackDepth;
   struct gl_attrib_node *ClientAttribStack[MAX_CLIENT_ATTRIB_STACK_DEPTH];
   /*@}*/

   /** \name Client attribute groups */
   /*@{*/
   struct gl_array_attrib	Array;	/**< Vertex arrays */
   struct gl_pixelstore_attrib	Pack;	/**< Pixel packing */
   struct gl_pixelstore_attrib	Unpack;	/**< Pixel unpacking */
   struct gl_pixelstore_attrib	DefaultPacking;	/**< Default params */
   /*@}*/

   /** \name Other assorted state (not pushed/popped on attribute stack) */
   /*@{*/
   struct gl_pixelmaps          PixelMaps;
   struct gl_histogram_attrib	Histogram;
   struct gl_minmax_attrib	MinMax;
   struct gl_convolution_attrib Convolution1D;
   struct gl_convolution_attrib Convolution2D;
   struct gl_convolution_attrib Separable2D;

   struct gl_evaluators EvalMap;   /**< All evaluators */
   struct gl_feedback   Feedback;  /**< Feedback */
   struct gl_selection  Select;    /**< Selection */

   struct gl_color_table ColorTable[COLORTABLE_MAX];
   struct gl_color_table ProxyColorTable[COLORTABLE_MAX];
#if 0
   struct gl_color_table PostConvolutionColorTable;
   struct gl_color_table ProxyPostConvolutionColorTable;
   struct gl_color_table PostColorMatrixColorTable;
   struct gl_color_table ProxyPostColorMatrixColorTable;
#endif

   struct gl_program_state Program;        /**< for vertex or fragment progs */
   struct gl_vertex_program_state VertexProgram;   /**< GL_ARB/NV_vertex_program */
   struct gl_fragment_program_state FragmentProgram;  /**< GL_ARB/NV_vertex_program */
   struct gl_ati_fragment_shader_state ATIFragmentShader;  /**< GL_ATI_fragment_shader */

   struct gl_query_state Query;  /**< GL_ARB_occlusion_query */

   struct gl_shader_state Shader; /**< GLSL shader object state */
   /*@}*/

#if FEATURE_EXT_framebuffer_object
   struct gl_renderbuffer *CurrentRenderbuffer;
#endif

   GLenum ErrorValue;        /**< Last error code */
   GLenum RenderMode;        /**< either GL_RENDER, GL_SELECT, GL_FEEDBACK */
   GLbitfield NewState;      /**< bitwise-or of _NEW_* flags */

   /** \name Derived state */
   /*@{*/
   /** Bitwise-or of DD_* flags.  Note that this bitfield may be used before
    * state validation so they need to always be current.
    */
   GLbitfield _TriangleCaps;
   GLbitfield _ImageTransferState;/**< bitwise-or of IMAGE_*_BIT flags */
   GLfloat _EyeZDir[3];
   GLfloat _ModelViewInvScale;
   GLboolean _NeedEyeCoords;
   GLboolean _ForceEyeCoords; 
   GLenum _CurrentProgram;    /* currently executing program */

   GLuint TextureStateTimestamp; /* detect changes to shared state */

   struct gl_shine_tab *_ShineTable[2]; /**< Active shine tables */
   struct gl_shine_tab *_ShineTabList;  /**< MRU list of inactive shine tables */
   /**@}*/

   struct gl_list_extensions ListExt; /**< driver dlist extensions */

   /** \name For debugging/development only */
   /*@{*/
   GLboolean FirstTimeCurrent;
   /*@}*/

   /** Dither disable via MESA_NO_DITHER env var */
   GLboolean NoDither;

   /** software compression/decompression supported or not */
   GLboolean Mesa_DXTn;

   /** Core tnl module support */
   struct gl_tnl_module TnlModule;

   /**
    * \name Hooks for module contexts.  
    *
    * These will eventually live in the driver or elsewhere.
    */
   /*@{*/
   void *swrast_context;
   void *swsetup_context;
   void *swtnl_context;
   void *swtnl_im;
   void *acache_context;
   void *aelt_context;
   /*@}*/
};


/** The string names for GL_POINT, GL_LINE_LOOP, etc */
extern const char *_mesa_prim_name[GL_POLYGON+4];


#ifdef DEBUG
extern int MESA_VERBOSE;
extern int MESA_DEBUG_FLAGS;
# define MESA_FUNCTION __FUNCTION__
#else
# define MESA_VERBOSE 0
# define MESA_DEBUG_FLAGS 0
# define MESA_FUNCTION "a function"
# ifndef NDEBUG
#  define NDEBUG
# endif
#endif


enum _verbose
{
   VERBOSE_VARRAY		= 0x0001,
   VERBOSE_TEXTURE		= 0x0002,
   VERBOSE_IMMEDIATE		= 0x0004,
   VERBOSE_PIPELINE		= 0x0008,
   VERBOSE_DRIVER		= 0x0010,
   VERBOSE_STATE		= 0x0020,
   VERBOSE_API			= 0x0040,
   VERBOSE_DISPLAY_LIST		= 0x0100,
   VERBOSE_LIGHTING		= 0x0200,
   VERBOSE_PRIMS		= 0x0400,
   VERBOSE_VERTS		= 0x0800,
   VERBOSE_DISASSEM		= 0x1000
};


enum _debug
{
   DEBUG_ALWAYS_FLUSH		= 0x1
};



#define Elements(x) sizeof(x)/sizeof(*(x))


#endif /* TYPES_H */
