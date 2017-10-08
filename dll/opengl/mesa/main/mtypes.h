/*
 * Mesa 3-D graphics library
 * Version:  7.7
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * \file mtypes.h
 * Main Mesa data structures.
 *
 * Please try to mark derived values with a leading underscore ('_').
 */

#ifndef MTYPES_H
#define MTYPES_H


#include "main/glheader.h"
#include "main/config.h"
#include "main/mfeatures.h"
#include "math/m_matrix.h"	/* GLmatrix */
#include "main/simple_list.h"	/* struct simple_node */
#include "main/formats.h"       /* MESA_FORMAT_COUNT */


#ifdef __cplusplus
extern "C" {
#endif


/**
 * \name 64-bit extension of GLbitfield.
 */
/*@{*/
typedef GLuint64 GLbitfield64;

/** Set a single bit */
#define BITFIELD64_BIT(b)      ((GLbitfield64)1 << (b))
/** Set all bits up to excluding bit b */
#define BITFIELD64_MASK(b)      \
   ((b) == 64 ? (~(GLbitfield64)0) : BITFIELD64_BIT(b) - 1)
/** Set count bits starting from bit b  */
#define BITFIELD64_RANGE(b, count) \
   (BITFIELD64_MASK((b) + (count)) & ~BITFIELD64_MASK(b))


/**
 * \name Some forward type declarations
 */
/*@{*/
struct _mesa_HashTable;
struct gl_attrib_node;
struct gl_list_extensions;
struct gl_meta_state;
struct gl_texture_object;
struct gl_context;
struct st_context;
/*@}*/


/** Extra draw modes beyond GL_POINTS, GL_TRIANGLE_FAN, etc */
#define PRIM_OUTSIDE_BEGIN_END   (GL_POLYGON+1)
#define PRIM_INSIDE_UNKNOWN_PRIM (GL_POLYGON+2)
#define PRIM_UNKNOWN             (GL_POLYGON+3)



/**
 * Indexes for vertex program attributes.
 * GL_NV_vertex_program aliases generic attributes over the conventional
 * attributes.  In GL_ARB_vertex_program shader the aliasing is optional.
 * In GL_ARB_vertex_shader / OpenGL 2.0 the aliasing is disallowed (the
 * generic attributes are distinct/separate).
 */
typedef enum
{
   VERT_ATTRIB_POS = 0,
   VERT_ATTRIB_WEIGHT = 1,
   VERT_ATTRIB_NORMAL = 2,
   VERT_ATTRIB_COLOR = 3,
   VERT_ATTRIB_FOG = 4,
   VERT_ATTRIB_COLOR_INDEX = 5,
   VERT_ATTRIB_EDGEFLAG = 6,
   VERT_ATTRIB_TEX = 7,
   VERT_ATTRIB_POINT_SIZE = 8,
   VERT_ATTRIB_MAX = 9
} gl_vert_attrib;

/**
 * Symbolic constats to help iterating over
 * specific blocks of vertex attributes.
 *
 */
#define VERT_ATTRIB(i)           (VERT_ATTRIB_POS + (i))

/**
 * Bitflags for vertex attributes.
 * These are used in bitfields in many places.
 */
/*@{*/
#define VERT_BIT_POS             BITFIELD64_BIT(VERT_ATTRIB_POS)
#define VERT_BIT_WEIGHT          BITFIELD64_BIT(VERT_ATTRIB_WEIGHT)
#define VERT_BIT_NORMAL          BITFIELD64_BIT(VERT_ATTRIB_NORMAL)
#define VERT_BIT_COLOR           BITFIELD64_BIT(VERT_ATTRIB_COLOR)
#define VERT_BIT_FOG             BITFIELD64_BIT(VERT_ATTRIB_FOG)
#define VERT_BIT_COLOR_INDEX     BITFIELD64_BIT(VERT_ATTRIB_COLOR_INDEX)
#define VERT_BIT_EDGEFLAG        BITFIELD64_BIT(VERT_ATTRIB_EDGEFLAG)
#define VERT_BIT_TEX             BITFIELD64_BIT(VERT_ATTRIB_TEX)
#define VERT_BIT_POINT_SIZE      BITFIELD64_BIT(VERT_ATTRIB_POINT_SIZE)

#define VERT_BIT(i)              BITFIELD64_BIT(i)
#define VERT_BIT_ALL             (BITFIELD64_BIT(VERT_ATTRIB_MAX) - 1)

/*@}*/


/**
 * Indexes for fragment program input attributes.  Note that
 * _mesa_vert_result_to_frag_attrib() and frag_attrib_to_vert_result() make
 * assumptions about the layout of this enum.
 */
typedef enum
{
   FRAG_ATTRIB_WPOS = 0,
   FRAG_ATTRIB_COL = 1,
   FRAG_ATTRIB_FOGC = 2,
   FRAG_ATTRIB_TEX = 3,
   FRAG_ATTRIB_FACE = 4,  /**< front/back face */
   FRAG_ATTRIB_PNTC = 5,  /**< sprite/point coord */
   FRAG_ATTRIB_CLIP_DIST0 = 6,
   FRAG_ATTRIB_CLIP_DIST1 = 7,
   FRAG_ATTRIB_MAX = 8
} gl_frag_attrib;


#define FRAG_BIT_COL  (1 << FRAG_ATTRIB_COL)
#define FRAG_BIT_FOGC  (1 << FRAG_ATTRIB_FOGC)
#define FRAG_BIT_TEX  (1 << FRAG_ATTRIB_TEX)

/**
 * Indexes for all renderbuffers
 */
typedef enum
{
   /* the four standard color buffers */
   BUFFER_FRONT_LEFT,
   BUFFER_BACK_LEFT,
   BUFFER_FRONT_RIGHT,
   BUFFER_BACK_RIGHT,
   BUFFER_DEPTH,
   BUFFER_STENCIL,
   BUFFER_ACCUM,
   /* optional aux buffer */
   BUFFER_AUX0,
   BUFFER_COUNT
} gl_buffer_index;

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

/**
 * Mask of all the color buffer bits (but not accum).
 */
#define BUFFER_BITS_COLOR  (BUFFER_BIT_FRONT_LEFT | \
                            BUFFER_BIT_BACK_LEFT | \
                            BUFFER_BIT_FRONT_RIGHT | \
                            BUFFER_BIT_BACK_RIGHT | \
                            BUFFER_BIT_AUX0)


/**
 * Framebuffer configuration (aka visual / pixelformat)
 * Note: some of these fields should be boolean, but it appears that
 * code in drivers/dri/common/util.c requires int-sized fields.
 */
struct gl_config
{
   GLboolean rgbMode;
   GLboolean colorIndexMode;  /* XXX is this used anywhere? */
   GLuint doubleBufferMode;
   GLuint stereoMode;

   GLboolean haveAccumBuffer;
   GLboolean haveDepthBuffer;
   GLboolean haveStencilBuffer;

   GLint redBits, greenBits, blueBits, alphaBits;	/* bits per comp */
   GLuint redMask, greenMask, blueMask, alphaMask;
   GLint rgbBits;		/* total bits for rgb */
   GLint indexBits;		/* total bits for colorindex */

   GLint accumRedBits, accumGreenBits, accumBlueBits, accumAlphaBits;
   GLint depthBits;
   GLint stencilBits;

   GLint numAuxBuffers;

   GLint level;

   /* EXT_visual_rating / GLX 1.2 */
   GLint visualRating;

   /* EXT_visual_info / GLX 1.2 */
   GLint transparentPixel;
   /*    colors are floats scaled to ints */
   GLint transparentRed, transparentGreen, transparentBlue, transparentAlpha;
   GLint transparentIndex;

   /* SGIX_pbuffer / GLX 1.3 */
   GLint maxPbufferWidth;
   GLint maxPbufferHeight;
   GLint maxPbufferPixels;
   GLint optimalPbufferWidth;   /* Only for SGIX_pbuffer. */
   GLint optimalPbufferHeight;  /* Only for SGIX_pbuffer. */

   /* OML_swap_method */
   GLint swapMethod;

   /* EXT_texture_from_pixmap */
   GLint bindToTextureRgb;
   GLint bindToTextureRgba;
   GLint bindToMipmapTexture;
   GLint bindToTextureTargets;
   GLint yInverted;
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
   GLfloat SpotDirection[4];	/**< spotlight direction in eye coordinates */
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
   GLfloat _NormSpotDirection[4]; /**< normalized spotlight direction */
   GLfloat _VP_inf_spot_attenuation;

   GLfloat _SpotExpTable[EXP_TABLE_SIZE][2];  /**< to replace a pow() call */
   GLfloat _MatAmbient[2][3];	/**< material ambient * light ambient */
   GLfloat _MatDiffuse[2][3];	/**< material diffuse * light diffuse */
   GLfloat _MatSpecular[2][3];	/**< material spec * light specular */
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
 * Used for storing clear color, texture border color, etc.
 * The float values are typically unclamped.
 */
union gl_color_union
{
   GLfloat f[4];
   GLint i[4];
   GLuint ui[4];
};


/**
 * Color buffer attribute group (GL_COLOR_BUFFER_BIT).
 */
struct gl_colorbuffer_attrib
{
   GLuint ClearIndex;                      /**< Index for glClear */
   union gl_color_union ClearColor;        /**< Color for glClear, unclamped */
   GLuint IndexMask;                       /**< Color index write mask */
   GLubyte ColorMask[4]; /**< Each flag is 0xff or 0x0 */

   GLenum DrawBuffer;	/**< Which buffer to draw into */

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
   GLbitfield BlendEnabled;		/**< Per-buffer blend enable flags */

   /* NOTE: this does _not_ depend on fragment clamping or any other clamping
    * control, only on the fixed-pointness of the render target.
    * The query does however depend on fragment color clamping.
    */

   GLenum SrcFactor;             /**< RGB blend source term */
   GLenum DstFactor;             /**< RGB blend dest term */
   /*@}*/

   /** 
    * \name Logic op
    */
   /*@{*/
   GLenum LogicOp;			/**< Logic operator */
   GLboolean IndexLogicOpEnabled;	/**< Color index logic op enabled flag */
   GLboolean ColorLogicOpEnabled;	/**< RGBA logic op enabled flag */
   /*@}*/

   GLboolean DitherFlag;		/**< Dither enable flag */
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
   GLfloat Attrib[VERT_ATTRIB_MAX][4];	/**< Position, color, texcoords, etc */

   /**
    * \name Current raster position attributes (always valid).
    * \note This set of attributes is very similar to the SWvertex struct.
    */
   /*@{*/
   GLfloat RasterPos[4];
   GLfloat RasterDistance;
   GLfloat RasterColor[4];
   GLfloat RasterTexCoords[4];
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
   GLboolean Map2Color4;
   GLboolean Map2Index;
   GLboolean Map2Normal;
   GLboolean Map2TextureCoord1;
   GLboolean Map2TextureCoord2;
   GLboolean Map2TextureCoord3;
   GLboolean Map2TextureCoord4;
   GLboolean Map2Vertex3;
   GLboolean Map2Vertex4;
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
   GLenum FogCoordinateSource;  /**< GL_EXT_fog_coord */
   GLfloat _Scale;		/**< (End == Start) ? 1.0 : 1.0 / (End - Start) */
   GLenum FogDistanceMode;     /**< GL_NV_fog_distance */
};


/**
 * \brief Layout qualifiers for gl_FragDepth.
 *
 * Extension AMD_conservative_depth allows gl_FragDepth to be redeclared with
 * a layout qualifier.
 *
 * \see enum ir_depth_layout
 */
enum gl_frag_depth_layout {
    FRAG_DEPTH_LAYOUT_NONE, /**< No layout is specified. */
    FRAG_DEPTH_LAYOUT_ANY,
    FRAG_DEPTH_LAYOUT_GREATER,
    FRAG_DEPTH_LAYOUT_LESS,
    FRAG_DEPTH_LAYOUT_UNCHANGED
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

   struct gl_light EnabledList;         /**< List sentinel */

   /** 
    * Derived state for optimizations: 
    */
   /*@{*/
   GLboolean _NeedEyeCoords;		
   GLboolean _NeedVertices;		/**< Use fast shader? */
   GLbitfield _Flags;		        /**< LIGHT_* flags, see above */
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

   /** Scale & Bias (index shift, offset) */
   /*@{*/
   GLfloat RedBias, RedScale;
   GLfloat GreenBias, GreenScale;
   GLfloat BlueBias, BlueScale;
   GLfloat AlphaBias, AlphaScale;
   GLfloat DepthBias, DepthScale;
   GLint IndexShift, IndexOffset;
   /*@}*/

   /* Pixel Maps */
   /* Note: actual pixel maps are not part of this attrib group */
   GLboolean MapColorFlag;
   GLboolean MapStencilFlag;

   /*--- End Pixel Transfer State ---*/

   /** glPixelZoom */
   GLfloat ZoomX, ZoomY;
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
   GLboolean CoordReplace; /**< GL_ARB_point_sprite*/
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
 *
 */
struct gl_stencil_attrib
{
   GLboolean Enabled;		/**< Enabled flag */
   GLboolean _Enabled;          /**< Enabled and stencil buffer present */
   GLenum Function;		/**< Stencil function */
   GLenum FailFunc;		/**< Fail function */
   GLenum ZPassFunc;		/**< Depth buffer pass function */
   GLenum ZFailFunc;		/**< Depth buffer fail function */
   GLint Ref;		/**< Reference value */
   GLuint ValueMask;		/**< Value mask */
   GLuint WriteMask;		/**< Write mask */
   GLuint Clear;		/**< Clear value */
};


/**
 * An index for each type of texture object.  These correspond to the GL
 * texture target enums, such as GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, etc.
 * Note: the order is from highest priority to lowest priority.
 */
typedef enum
{
   TEXTURE_CUBE_INDEX,
   TEXTURE_2D_INDEX,
   TEXTURE_1D_INDEX,
   NUM_TEXTURE_TARGETS
} gl_texture_index;


/**
 * Bit flags for each type of texture object
 * Used for Texture.Unit[]._ReallyEnabled flags.
 */
/*@{*/
#define TEXTURE_CUBE_BIT     (1 << TEXTURE_CUBE_INDEX)
#define TEXTURE_2D_BIT       (1 << TEXTURE_2D_INDEX)
#define TEXTURE_1D_BIT       (1 << TEXTURE_1D_INDEX)
/*@}*/


/**
 * TexGenEnabled flags.
 */
/*@{*/
#define S_BIT 1
#define T_BIT 2
#define R_BIT 4
#define Q_BIT 8
#define STR_BITS (S_BIT | T_BIT | R_BIT)
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



/** Tex-gen enabled for texture unit? */
#define ENABLE_TEXGEN(unit) (1 << (unit))

/** Non-identity texture matrix for texture unit? */
#define ENABLE_TEXMAT(unit) (1 << (unit))


/**
 * Texture image state.  Drivers will typically create a subclass of this
 * with extra fields for memory buffers, etc.
 */
struct gl_texture_image
{
   GLint InternalFormat;	/**< Internal format as given by the user */
   GLenum _BaseFormat;		/**< Either GL_RGB, GL_RGBA, GL_ALPHA,
				 *   GL_LUMINANCE, GL_LUMINANCE_ALPHA,
				 *   GL_INTENSITY, GL_DEPTH_COMPONENT or
				 *   GL_DEPTH_STENCIL_EXT only. Used for
				 *   choosing TexEnv arithmetic.
				 */
   gl_format TexFormat;         /**< The actual texture memory format */

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

   struct gl_texture_object *TexObject;  /**< Pointer back to parent object */
   GLuint Level;                /**< Which mipmap level am I? */
   /** Cube map face: index into gl_texture_object::Image[] array */
   GLuint Face;
};


/**
 * Indexes for cube map faces.
 */
typedef enum
{
   FACE_POS_X = 0,
   FACE_NEG_X = 1,
   FACE_POS_Y = 2,
   FACE_NEG_Y = 3,
   FACE_POS_Z = 4,
   FACE_NEG_Z = 5,
   MAX_FACES = 6
} gl_face_index;

/**
 * Sampler object state.  These objects are new with GL_ARB_sampler_objects
 * and OpenGL 3.3.  Legacy texture objects also contain a sampler object.
 */
struct gl_sampler_object
{
   GLuint Name;
   GLint RefCount;

   GLenum WrapS;		/**< S-axis texture image wrap mode */
   GLenum WrapT;		/**< T-axis texture image wrap mode */
   GLenum WrapR;		/**< R-axis texture image wrap mode */
   GLenum MinFilter;		/**< minification filter */
   GLenum MagFilter;		/**< magnification filter */
   union gl_color_union BorderColor;  /**< Interpreted according to texture format */
   GLfloat MaxAnisotropy;	/**< GL_EXT_texture_filter_anisotropic */

};


/**
 * Texture object state.  Contains the array of mipmap images, border color,
 * wrap modes, filter modes, and shadow/texcompare state.
 */
struct gl_texture_object
{
   _glthread_Mutex Mutex;	/**< for thread safety */
   GLint RefCount;		/**< reference count */
   GLuint Name;			/**< the user-visible texture object ID */
   GLenum Target;               /**< GL_TEXTURE_1D, GL_TEXTURE_2D, etc. */

   struct gl_sampler_object Sampler;

   GLfloat Priority;		/**< in [0,1] */
   GLint BaseLevel;		/**< min mipmap level, OpenGL 1.2 */
   GLint MaxLevel;		/**< max mipmap level, OpenGL 1.2 */
   GLint _MaxLevel;		/**< actual max mipmap level (q in the spec) */
   GLfloat _MaxLambda;		/**< = _MaxLevel - BaseLevel (q - b in spec) */
   GLboolean _Complete;		/**< Is texture object complete? */
   GLboolean Purgeable;         /**< Is the buffer purgeable under memory pressure? */
   GLboolean Immutable;         /**< GL_ARB_texture_storage */

   /** Actual texture images, indexed by [cube face] and [mipmap level] */
   struct gl_texture_image *Image[MAX_FACES][MAX_TEXTURE_LEVELS];
};


/** Up to four combiner sources are possible with GL_NV_texture_env_combine4 */
#define MAX_COMBINER_TERMS 4


/**
 * Texture combine environment state.
 */
struct gl_tex_env_combine_state
{
   GLenum ModeRGB;       /**< GL_REPLACE, GL_DECAL, GL_ADD, etc. */
   GLenum ModeA;         /**< GL_REPLACE, GL_DECAL, GL_ADD, etc. */
   /** Source terms: GL_PRIMARY_COLOR, GL_TEXTURE, etc */
   GLenum SourceRGB[MAX_COMBINER_TERMS];
   GLenum SourceA[MAX_COMBINER_TERMS];
   /** Source operands: GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, etc */
   GLenum OperandRGB[MAX_COMBINER_TERMS];
   GLenum OperandA[MAX_COMBINER_TERMS];
   GLuint ScaleShiftRGB; /**< 0, 1 or 2 */
   GLuint ScaleShiftA;   /**< 0, 1 or 2 */
   GLuint _NumArgsRGB;   /**< Number of inputs used for the RGB combiner */
   GLuint _NumArgsA;     /**< Number of inputs used for the A combiner */
};


/**
 * Texture coord generation state.
 */
struct gl_texgen
{
   GLenum Mode;         /**< GL_EYE_LINEAR, GL_SPHERE_MAP, etc */
   GLbitfield _ModeBit; /**< TEXGEN_x bit corresponding to Mode */
   GLfloat ObjectPlane[4];
   GLfloat EyePlane[4];
};


/**
 * Texture unit state.  Contains enable flags, texture environment/function/
 * combiners, texgen state, and pointers to current texture objects.
 */
struct gl_texture_unit
{
   GLbitfield Enabled;          /**< bitmask of TEXTURE_*_BIT flags */
   GLbitfield _ReallyEnabled;   /**< 0 or exactly one of TEXTURE_*_BIT flags */

   GLenum EnvMode;              /**< GL_MODULATE, GL_DECAL, GL_BLEND, etc. */
   GLclampf EnvColor[4];

   struct gl_texgen GenS;
   struct gl_texgen GenT;
   struct gl_texgen GenR;
   struct gl_texgen GenQ;
   GLbitfield TexGenEnabled;	/**< Bitwise-OR of [STRQ]_BIT values */
   GLbitfield _GenFlags;	/**< Bitwise-OR of Gen[STRQ]._ModeBit */

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

   /** Current texture object pointers */
   struct gl_texture_object *CurrentTex[NUM_TEXTURE_TARGETS];

   /** Points to highest priority, complete and enabled texture object */
   struct gl_texture_object *_Current;
};


/**
 * Texture attribute group (GL_TEXTURE_BIT).
 */
struct gl_texture_attrib
{
   struct gl_texture_unit Unit;

   struct gl_texture_object *ProxyTex[NUM_TEXTURE_TARGETS];
   
   GLboolean _Enabled;

   /** Texture coord units/sets used for fragment texturing */
   GLboolean _EnabledCoord;

   /** Texture coord units that have texgen enabled */
   GLboolean _TexGenEnabled;

   /** Texture coord units that have non-identity matrices */
   GLboolean _TexMatEnabled;

   /** Bitwise-OR of all Texture.Unit[i]._GenFlags */
   GLbitfield _GenFlags;
};


/**
 * Data structure representing a single clip plane (e.g. one of the elements
 * of the ctx->Transform.EyeUserPlane or ctx->Transform._ClipUserPlane array).
 */
typedef GLfloat gl_clip_plane[4];


/**
 * Transformation attribute group (GL_TRANSFORM_BIT).
 */
struct gl_transform_attrib
{
   GLenum MatrixMode;				/**< Matrix mode */
   gl_clip_plane EyeUserPlane[MAX_CLIP_PLANES];	/**< User clip planes */
   gl_clip_plane _ClipUserPlane[MAX_CLIP_PLANES]; /**< derived */
   GLbitfield ClipPlanesEnabled;                /**< on/off bitmask */
   GLboolean Normalize;				/**< Normalize all normals? */
   GLboolean RescaleNormals;			/**< GL_EXT_rescale_normal */
   GLboolean RasterPositionUnclipped;           /**< GL_IBM_rasterpos_clip */

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
 * GL_ARB_vertex/pixel_buffer_object buffer object
 */
struct gl_buffer_object
{
   _glthread_Mutex Mutex;
   GLint RefCount;
   GLuint Name;
   GLenum Usage;        /**< GL_STREAM_DRAW_ARB, GL_STREAM_READ_ARB, etc. */
   GLsizeiptrARB Size;  /**< Size of buffer storage in bytes */
   GLubyte *Data;       /**< Location of storage either in RAM or VRAM. */
   /** Fields describing a mapped buffer */
   /*@{*/
   GLbitfield AccessFlags; /**< Mask of GL_MAP_x_BIT flags */
   GLvoid *Pointer;     /**< User-space address of mapping */
   GLintptr Offset;     /**< Mapped offset */
   GLsizeiptr Length;   /**< Mapped length */
   /*@}*/
   GLboolean DeletePending;   /**< true if buffer object is removed from the hash */
   GLboolean Written;   /**< Ever written to? (for debugging) */
   GLboolean Purgeable; /**< Is the buffer purgeable under memory pressure? */
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
   GLint ImageHeight;
   GLint SkipImages;
   GLboolean SwapBytes;
   GLboolean LsbFirst;
   GLboolean Invert;        /**< GL_MESA_pack_invert */
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
   GLboolean Integer;           /**< Integer-valued? */
   GLuint _ElementSize;         /**< size of each element in bytes */

   struct gl_buffer_object *BufferObj;/**< GL_ARB_vertex_buffer_object */
   GLuint _MaxElement;          /**< max element index into array buffer + 1 */
};


/**
 * Vertex array state
 */
struct gl_array_attrib
{
    /** Vertex attribute arrays */
    struct gl_client_array VertexAttrib[VERT_ATTRIB_MAX];

    /** Mask of VERT_BIT_* values indicating which arrays are enabled */
    GLbitfield64 _Enabled;

    /**
     * Min of all enabled arrays' _MaxElement.  When arrays reside inside VBOs
     * we can determine the max legal (in bounds) glDrawElements array index.
     */
    GLuint _MaxElement;

    struct gl_buffer_object *ElementArrayBufferObj;

    GLuint LockFirst;            /**< GL_EXT_compiled_vertex_array */
    GLuint LockCount;            /**< GL_EXT_compiled_vertex_array */

    GLbitfield64 NewState;		/**< mask of VERT_BIT_* values */
    GLboolean RebindArrays; /**< whether the VBO module should rebind arrays */
};


/**
 * Feedback buffer state
 */
struct gl_feedback
{
   GLenum Type;
   GLbitfield _Mask;    /**< FB_* bits */
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
   /*@}*/
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

   /** Default texture objects (shared by all texture units) */
   struct gl_texture_object *DefaultTex[NUM_TEXTURE_TARGETS];

   /** Fallback texture used when a bound texture is incomplete */
   struct gl_texture_object *FallbackTex;

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

   /** Default buffer object for vertex arrays that aren't in VBOs */
   struct gl_buffer_object *NullBufferObj;

   struct _mesa_HashTable *BufferObjects;

   void *DriverData;  /**< Device driver shared state */
};



/**
 * Renderbuffers represent drawing surfaces such as color, depth and/or
 * stencil.  A framebuffer object has a set of renderbuffers.
 * Drivers will typically derive subclasses of this type.
 */
struct gl_renderbuffer
{
   _glthread_Mutex Mutex; /**< for thread safety */
   GLuint ClassID;        /**< Useful for drivers */
   GLint RefCount;
   GLuint Width, Height;
   GLboolean Purgeable;  /**< Is the buffer purgeable under memory pressure? */
   GLboolean AttachedAnytime; /**< TRUE if it was attached to a framebuffer */
   GLenum InternalFormat; /**< The user-specified format */
   GLenum _BaseFormat;    /**< Either GL_RGB, GL_RGBA, GL_DEPTH_COMPONENT or
                               GL_STENCIL_INDEX. */
   gl_format Format;      /**< The actual renderbuffer memory format */

   /** Delete this renderbuffer */
   void (*Delete)(struct gl_renderbuffer *rb);

   /** Allocate new storage for this renderbuffer */
   GLboolean (*AllocStorage)(struct gl_context *ctx,
                             struct gl_renderbuffer *rb,
                             GLenum internalFormat,
                             GLuint width, GLuint height);
};


/**
 * A renderbuffer attachment points to either a texture object (and specifies
 * a mipmap level, cube face or 3D texture slice) or points to a renderbuffer.
 */
struct gl_renderbuffer_attachment
{
   /**
    * If \c Type is \c GL_RENDERBUFFER_EXT, this stores a pointer to the
    * application supplied renderbuffer object.
    */
   struct gl_renderbuffer *Renderbuffer;
};


/**
 * A framebuffer is a collection of renderbuffers (color, depth, stencil, etc).
 * In C++ terms, think of this as a base class from which device drivers
 * will make derived classes.
 */
struct gl_framebuffer
{
   _glthread_Mutex Mutex;  /**< for thread safety */

   GLint RefCount;
   GLboolean DeletePending;

   /**
    * The framebuffer's visual. Immutable if this is a window system buffer.
    * Computed from attachments if user-made FBO.
    */
   struct gl_config Visual;

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

   /** Integer color values */
   GLboolean _IntegerColor;

   /** Array of all renderbuffer attachments, indexed by BUFFER_* tokens. */
   struct gl_renderbuffer_attachment Attachment[BUFFER_COUNT];

   /* In unextended OpenGL these vars are part of the GL_COLOR_BUFFER
    * attribute group and GL_PIXEL attribute group, respectively.
    */
   GLenum ColorDrawBuffer;
   GLenum ColorReadBuffer;

   /** Computed from ColorDraw/ReadBuffer above */
   GLint _ColorDrawBufferIndex; /**< BUFFER_x or -1 */
   GLint _ColorReadBufferIndex; /* -1 = None */
   struct gl_renderbuffer *_ColorDrawBuffer;
   struct gl_renderbuffer *_ColorReadBuffer;

   /** Delete this framebuffer */
   void (*Delete)(struct gl_framebuffer *fb);
};

/**
 * Constants which may be overridden by device driver during context creation
 * but are never changed after that.
 */
struct gl_constants
{
   GLint MaxTextureMbytes;      /**< Max memory per image, in MB */
   GLint MaxTextureLevels;      /**< Max mipmap levels. */
   GLint MaxCubeTextureLevels;  /**< Max mipmap levels for cube textures */
   GLfloat MaxTextureMaxAnisotropy;  /**< GL_EXT_texture_filter_anisotropic */

   GLuint MaxArrayLockSize;

   GLint SubPixelBits;

   GLfloat MinPointSize, MaxPointSize;	     /**< aliased */
   GLfloat MinPointSizeAA, MaxPointSizeAA;   /**< antialiased */
   GLfloat PointSizeGranularity;
   GLfloat MinLineWidth, MaxLineWidth;       /**< aliased */
   GLfloat MinLineWidthAA, MaxLineWidthAA;   /**< antialiased */
   GLfloat LineWidthGranularity;

   GLuint MaxClipPlanes;
   GLuint MaxLights;
   GLfloat MaxShininess;                     /**< GL_NV_light_max_exponent */
   GLfloat MaxSpotExponent;                  /**< GL_NV_light_max_exponent */

   GLuint MaxViewportWidth, MaxViewportHeight;

   /** vertex array / buffer object bounds checking */
   GLboolean CheckArrayBounds;

   /**
    * Does the driver support real 32-bit integers?  (Otherwise, integers are
    * simulated via floats.)
    */
   GLboolean NativeIntegers;

   /** OpenGL version 3.0 */
   GLbitfield ContextFlags;  /**< Ex: GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT */

   /** OpenGL version 3.2 */
   GLbitfield ProfileMask;   /**< Mask of CONTEXT_x_PROFILE_BIT */

   /**
    * Whether the implementation strips out and ignores texture borders.
    *
    * Many GPU hardware implementations don't support rendering with texture
    * borders and mipmapped textures.  (Note: not static border color, but the
    * old 1-pixel border around each edge).  Implementations then have to do
    * slow fallbacks to be correct, or just ignore the border and be fast but
    * wrong.  Setting the flag stripts the border off of TexImage calls,
    * providing "fast but wrong" at significantly reduced driver complexity.
    *
    * Texture borders are deprecated in GL 3.0.
    **/
   GLboolean StripTextureBorder;
};


/**
 * Enable flag for each OpenGL extension.  Different device drivers will
 * enable different extensions at runtime.
 */
struct gl_extensions
{
   GLboolean dummy;  /* don't remove this! */
   GLboolean dummy_true;  /* Set true by _mesa_init_extensions(). */
   GLboolean dummy_false; /* Set false by _mesa_init_extensions(). */
   GLboolean ARB_map_buffer_range;
   GLboolean ARB_point_sprite;
   GLboolean ARB_texture_cube_map;
   GLboolean ARB_texture_env_combine;
   GLboolean ARB_texture_env_crossbar;
   GLboolean ARB_texture_env_dot3;
   GLboolean ARB_texture_storage;
   GLboolean ARB_transpose_matrix;
   GLboolean ARB_window_pos;
   GLboolean EXT_blend_color;
   GLboolean EXT_blend_equation_separate;
   GLboolean EXT_blend_func_separate;
   GLboolean EXT_blend_minmax;
   GLboolean EXT_clip_volume_hint;
   GLboolean EXT_compiled_vertex_array;
   GLboolean EXT_depth_bounds_test;
   GLboolean EXT_draw_range_elements;
   GLboolean EXT_fog_coord;
   GLboolean EXT_packed_pixels;
   GLboolean EXT_point_parameters;
   GLboolean EXT_rescale_normal;
   GLboolean EXT_shadow_funcs;
   GLboolean EXT_secondary_color;
   GLboolean EXT_separate_shader_objects;
   GLboolean EXT_separate_specular_color;
   GLboolean EXT_texture_env_dot3;
   GLboolean EXT_texture_filter_anisotropic;
   GLboolean EXT_texture_integer;
   /* vendor extensions */
   GLboolean APPLE_packed_pixels;
   GLboolean ATI_texture_env_combine3;
   GLboolean IBM_rasterpos_clip;
   GLboolean IBM_multimode_draw_arrays;
   GLboolean MESA_pack_invert;
   GLboolean MESA_resize_buffers;
   GLboolean MESA_ycbcr_texture;
   GLboolean NV_blend_square;
   GLboolean NV_fog_distance;
   GLboolean NV_light_max_exponent;
   GLboolean NV_point_sprite;
   GLboolean NV_texgen_reflection;
   GLboolean NV_texture_env_combine4;
   GLboolean extension_sentinel;
   /** The extension string */
   const GLubyte *String;
   /** Number of supported extensions */
   GLuint Count;
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
 * \sa __struct gl_contextRec::ImageTransferState.
 */
/*@{*/
#define IMAGE_SCALE_BIAS_BIT                      0x1
#define IMAGE_SHIFT_OFFSET_BIT                    0x2
#define IMAGE_MAP_COLOR_BIT                       0x4
#define IMAGE_CLAMP_BIT                           0x800


/** Pixel Transfer ops */
#define IMAGE_BITS (IMAGE_SCALE_BIAS_BIT |			\
		    IMAGE_SHIFT_OFFSET_BIT |			\
		    IMAGE_MAP_COLOR_BIT)

/**
 * \name Bits to indicate what state has changed.  
 */
/*@{*/
#define _NEW_MODELVIEW         (1 << 0)   /**< gl_context::ModelView */
#define _NEW_PROJECTION        (1 << 1)   /**< gl_context::Projection */
#define _NEW_TEXTURE_MATRIX    (1 << 2)   /**< gl_context::TextureMatrix */
#define _NEW_COLOR             (1 << 3)   /**< gl_context::Color */
#define _NEW_DEPTH             (1 << 4)   /**< gl_context::Depth */
#define _NEW_EVAL              (1 << 5)   /**< gl_context::Eval, EvalMap */
#define _NEW_FOG               (1 << 6)   /**< gl_context::Fog */
#define _NEW_HINT              (1 << 7)   /**< gl_context::Hint */
#define _NEW_LIGHT             (1 << 8)   /**< gl_context::Light */
#define _NEW_LINE              (1 << 9)   /**< gl_context::Line */
#define _NEW_PIXEL             (1 << 10)  /**< gl_context::Pixel */
#define _NEW_POINT             (1 << 11)  /**< gl_context::Point */
#define _NEW_POLYGON           (1 << 12)  /**< gl_context::Polygon */
#define _NEW_POLYGONSTIPPLE    (1 << 13)  /**< gl_context::PolygonStipple */
#define _NEW_SCISSOR           (1 << 14)  /**< gl_context::Scissor */
#define _NEW_STENCIL           (1 << 15)  /**< gl_context::Stencil */
#define _NEW_TEXTURE           (1 << 16)  /**< gl_context::Texture */
#define _NEW_TRANSFORM         (1 << 17)  /**< gl_context::Transform */
#define _NEW_VIEWPORT          (1 << 18)  /**< gl_context::Viewport */
#define _NEW_PACKUNPACK        (1 << 19)  /**< gl_context::Pack, Unpack */
#define _NEW_ARRAY             (1 << 20)  /**< gl_context::Array */
#define _NEW_RENDERMODE        (1 << 21)  /**< gl_context::RenderMode, etc */
#define _NEW_BUFFERS           (1 << 22)  /**< gl_context::Visual, DrawBuffer, */
#define _NEW_CURRENT_ATTRIB    (1 << 23)  /**< gl_context::Current */
#define _NEW_MULTISAMPLE       (1 << 24)  /**< gl_context::Multisample */
#define _NEW_BUFFER_OBJECT     (1 << 25)
#define _NEW_ALL ~0

/**
 * We use _NEW_TRANSFORM for GL_RASTERIZER_DISCARD.  This #define is for
 * clarity.
 */
#define _NEW_RASTERIZER_DISCARD _NEW_TRANSFORM
/*@}*/


/**
 * \name A bunch of flags that we think might be useful to drivers.
 * 
 * Set in the __struct gl_contextRec::_TriangleCaps bitfield.
 */
/*@{*/
#define DD_FLATSHADE                0x1
#define DD_TRI_CULL_FRONT_BACK      0x2 /* special case on some hw */
#define DD_TRI_LIGHT_TWOSIDE        0x4
#define DD_TRI_UNFILLED             0x8
#define DD_TRI_SMOOTH               0x10
#define DD_TRI_STIPPLE              0x20
#define DD_TRI_OFFSET               0x40
#define DD_LINE_SMOOTH              0x80
#define DD_LINE_STIPPLE             0x100
#define DD_POINT_SMOOTH             0x200
#define DD_POINT_ATTEN              0x400
/*@}*/


/**
 * \name Define the state changes under which each of these bits might change
 */
/*@{*/
#define _DD_NEW_FLATSHADE                _NEW_LIGHT
#define _DD_NEW_SEPARATE_SPECULAR        (_NEW_LIGHT | _NEW_FOG)
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


/**
 * Composite state flags
 */
/*@{*/
#define _MESA_NEW_NEED_EYE_COORDS         (_NEW_LIGHT |		\
                                           _NEW_TEXTURE |	\
                                           _NEW_POINT |		\
                                           _NEW_MODELVIEW)
/*@}*/




/* This has to be included here. */
#include "dd.h"


/**
 * Display list flags.
 * Strictly this is a tnl-private concept, but it doesn't seem
 * worthwhile adding a tnl private structure just to hold this one bit
 * of information:
 */
#define DLIST_DANGLING_REFS     0x1 


/** Opaque declaration of display list payload data type */
union gl_dlist_node;


/**
 * Provide a location where information about a display list can be
 * collected.  Could be extended with driverPrivate structures,
 * etc. in the future.
 */
struct gl_display_list
{
   GLuint Name;
   GLbitfield Flags;  /**< DLIST_x flags */
   /** The dlist commands are in a linked list of nodes */
   union gl_dlist_node *Head;
};


/**
 * State used during display list compilation and execution.
 */
struct gl_dlist_state
{
   GLuint CallDepth;		/**< Current recursion calling depth */

   struct gl_display_list *CurrentList; /**< List currently being compiled */
   union gl_dlist_node *CurrentBlock; /**< Pointer to current block of nodes */
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

   struct {
      /* State known to have been set by the currently-compiling display
       * list.  Used to eliminate some redundant state changes.
       */
      GLenum ShadeModel;
   } Current;
};


/**
 * Mesa rendering context.
 *
 * This is the central context data structure for Mesa.  Almost all
 * OpenGL state is contained in this structure.
 * Think of this as a base class from which device drivers will derive
 * sub classes.
 *
 * The struct gl_context typedef names this structure.
 */
struct gl_context
{
   /** State possibly shared with other contexts in the address space */
   struct gl_shared_state *Shared;

   /** \name API function pointer tables */
   /*@{*/
   struct _glapi_table *Save;	/**< Display list save functions */
   struct _glapi_table *Exec;	/**< Execute functions */
   struct _glapi_table *CurrentDispatch;  /**< == Save or Exec !! */
   /*@}*/

   struct gl_config Visual;
   struct gl_framebuffer *DrawBuffer;	/**< buffer for writing */
   struct gl_framebuffer *ReadBuffer;	/**< buffer for reading */
   struct gl_framebuffer *WinSysDrawBuffer;  /**< set with MakeCurrent */
   struct gl_framebuffer *WinSysReadBuffer;  /**< set with MakeCurrent */

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
   struct gl_matrix_stack TextureMatrixStack;
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

   /** Version info */
   GLuint VersionMajor, VersionMinor;
   char *VersionString;

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

   struct gl_evaluators EvalMap;   /**< All evaluators */
   struct gl_feedback   Feedback;  /**< Feedback */
   struct gl_selection  Select;    /**< Selection */
   /*@}*/

   struct gl_meta_state *Meta;  /**< for "meta" operations */

   /* GL_EXT_framebuffer_object */
   struct gl_renderbuffer *CurrentRenderbuffer;

   GLenum ErrorValue;        /**< Last error code */

   /**
    * Recognize and silence repeated error debug messages in buggy apps.
    */
   const char *ErrorDebugFmtString;
   GLuint ErrorDebugCount;

   GLenum RenderMode;        /**< either GL_RENDER, GL_SELECT, GL_FEEDBACK */
   GLbitfield NewState;      /**< bitwise-or of _NEW_* flags */

   GLboolean ViewportInitialized;  /**< has viewport size been initialized? */

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

   GLuint TextureStateTimestamp; /**< detect changes to shared state */

   struct gl_shine_tab *_ShineTable[2]; /**< Active shine tables */
   struct gl_shine_tab *_ShineTabList;  /**< MRU list of inactive shine tables */
   /**@}*/

   struct gl_list_extensions *ListExt; /**< driver dlist extensions */

   /** \name For debugging/development only */
   /*@{*/
   GLboolean FirstTimeCurrent;
   /*@}*/

   GLboolean TextureFormatSupported[MESA_FORMAT_COUNT];

   /** 
    * Use dp4 (rather than mul/mad) instructions for position
    * transformation?
    */
   GLboolean mvp_with_dp4;

   GLboolean RasterDiscard;  /**< GL_RASTERIZER_DISCARD */

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
   struct st_context *st;
   void *aelt_context;
   /*@}*/
};


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


/** The MESA_VERBOSE var is a bitmask of these flags */
enum _verbose
{
   VERBOSE_VARRAY		= 0x0001,
   VERBOSE_TEXTURE		= 0x0002,
   VERBOSE_MATERIAL		= 0x0004,
   VERBOSE_PIPELINE		= 0x0008,
   VERBOSE_DRIVER		= 0x0010,
   VERBOSE_STATE		= 0x0020,
   VERBOSE_API			= 0x0040,
   VERBOSE_DISPLAY_LIST		= 0x0100,
   VERBOSE_LIGHTING		= 0x0200,
   VERBOSE_PRIMS		= 0x0400,
   VERBOSE_VERTS		= 0x0800,
   VERBOSE_DISASSEM		= 0x1000,
   VERBOSE_DRAW                 = 0x2000,
   VERBOSE_SWAPBUFFERS          = 0x4000
};


/** The MESA_DEBUG_FLAGS var is a bitmask of these flags */
enum _debug
{
   DEBUG_ALWAYS_FLUSH		= 0x1
};



#ifdef __cplusplus
}
#endif

#endif /* MTYPES_H */
