/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#ifndef _S3V_CONTEXT_H_
#define _S3V_CONTEXT_H_

#include "dri_util.h"

#include "s3v_dri.h"
#include "s3v_regs.h"
#include "s3v_macros.h"
#include "s3v_screen.h"
#include "colormac.h"
#include "macros.h"
#include "mtypes.h"
#include "drm.h"
#include "mm.h"
#include "drirenderbuffer.h"

/* Flags for context */
#define S3V_FRONT_BUFFER    0x00000001
#define S3V_BACK_BUFFER     0x00000002
#define S3V_DEPTH_BUFFER    0x00000004

	/* FIXME: check */
#define S3V_MAX_TEXTURE_SIZE    2048

/* These are the minimum requirements and should probably be increased */
#define MAX_MODELVIEW_STACK    16
#define MAX_PROJECTION_STACK    2
#define MAX_TEXTURE_STACK       2

extern void	  	s3vDDUpdateHWState(GLcontext *ctx);
extern s3vScreenPtr	s3vCreateScreen(__DRIscreenPrivate *sPriv);
extern void	  	s3vDestroyScreen(__DRIscreenPrivate *sPriv);
extern GLboolean 	s3vCreateContext(const __GLcontextModes *glVisual,
                                     __DRIcontextPrivate *driContextPriv,
                                     void *sharedContextPrivate);

#define S3V_UPLOAD_ALL			0xffffffff
/* #define S3V_UPLOAD_CLIPRECTS		0x00000002 */
#define S3V_UPLOAD_ALPHA		0x00000004
#define S3V_UPLOAD_BLEND		0x00000008
#define S3V_UPLOAD_DEPTH		0x00000010
#define S3V_UPLOAD_VIEWPORT		0x00000020
#define S3V_UPLOAD_SHADE		0x00000040
#define S3V_UPLOAD_CLIP			0x00000080
#define S3V_UPLOAD_MASKS		0x00000100
#define S3V_UPLOAD_WINDOW		0x00000200 /* defunct */
#define S3V_UPLOAD_GEOMETRY		0x00000400
#define S3V_UPLOAD_POLYGON		0x00000800
#define S3V_UPLOAD_DITHER		0x00001000
#define S3V_UPLOAD_LOGICOP		0x00002000
#define S3V_UPLOAD_FOG			0x00004000
#define S3V_UPLOAD_LIGHT		0x00008000
#define S3V_UPLOAD_CONTEXT		0x00010000
#define S3V_UPLOAD_TEX0			0x00020000
#define S3V_UPLOAD_STIPPLE		0x00040000
#define S3V_UPLOAD_TRANSFORM		0x00080000
#define S3V_UPLOAD_LINEMODE		0x00100000
#define S3V_UPLOAD_POINTMODE		0x00200000
#define S3V_UPLOAD_TRIMODE		0x00400000

#define S3V_NEW_CLIP			0x00000001
#define S3V_NEW_WINDOW			0x00000002
#define S3V_NEW_CONTEXT			0x00000004
#define S3V_NEW_TEXTURE			0x00000008 /* defunct */
#define S3V_NEW_ALPHA			0x00000010
#define S3V_NEW_DEPTH			0x00000020
#define S3V_NEW_MASKS			0x00000040
#define S3V_NEW_POLYGON			0x00000080
#define S3V_NEW_CULL			0x00000100
#define S3V_NEW_LOGICOP			0x00000200
#define S3V_NEW_FOG			0x00000400
#define S3V_NEW_LIGHT			0x00000800
#define S3V_NEW_STIPPLE			0x00001000
#define S3V_NEW_ALL			0xffffffff

#define S3V_FALLBACK_TRI		0x00000001
#define S3V_FALLBACK_TEXTURE		0x00000002

struct s3v_context;
typedef struct s3v_context s3vContextRec;
typedef struct s3v_context *s3vContextPtr;
typedef struct s3v_texture_object_t *s3vTextureObjectPtr;

#define VALID_S3V_TEXTURE_OBJECT(tobj)  (tobj) 

#define S3V_TEX_MAXLEVELS 12

/* For shared texture space managment, these texture objects may also
 * be used as proxies for regions of texture memory containing other
 * client's textures.  Such proxy textures (not to be confused with GL
 * proxy textures) are subject to the same LRU aging we use for our
 * own private textures, and thus we have a mechanism where we can
 * fairly decide between kicking out our own textures and those of
 * other clients.
 *
 * Non-local texture objects have a valid MemBlock to describe the
 * region managed by the other client, and can be identified by
 * 't->globj == 0' 
 */
struct s3v_texture_object_t {
	struct s3v_texture_object_t *next, *prev;

	GLuint age;   
	struct gl_texture_object *globj;
     
	int Pitch;
	int Height;
	int WidthLog2;
	int texelBytes;
	int totalSize;
	int bound;

	struct mem_block *MemBlock;   
	GLuint BufAddr;
   
	GLuint min_level;
	GLuint max_level;
	GLuint dirty_images;

	GLint firstLevel, lastLevel;  /* upload tObj->Image[first .. lastLevel] */

	struct { 
		const struct gl_texture_image *image;
      		int offset;		/* into BufAddr */
      		int height;
      		int internalFormat;
   	} image[S3V_TEX_MAXLEVELS];

	GLuint TextureCMD;

	GLuint TextureColorMode;
	GLuint TextureFilterMode;
	GLuint TextureBorderColor;
	GLuint TextureWrap;
	GLuint TextureMipSize;

	GLuint TextureBaseAddr[S3V_TEX_MAXLEVELS];
	GLuint TextureFormat;
	GLuint TextureReadMode;
};		

#define S3V_NO_PALETTE        0x0
#define S3V_USE_PALETTE       0x1
#define S3V_UPDATE_PALETTE    0x2
#define S3V_FALLBACK_PALETTE  0x4

void s3vUpdateTextureState( GLcontext *ctx );

void s3vDestroyTexObj( s3vContextPtr vmesa, s3vTextureObjectPtr t);
void s3vUploadTexImages( s3vContextPtr vmesa, s3vTextureObjectPtr t );

void s3vResetGlobalLRU( s3vContextPtr vmesa );
void s3vTexturesGone( s3vContextPtr vmesa, 
		       GLuint start, GLuint end, 
		       GLuint in_use ); 

void s3vEmitHwState( s3vContextPtr vmesa );
void s3vGetLock( s3vContextPtr vmesa, GLuint flags );
void s3vInitExtensions( GLcontext *ctx );
void s3vInitDriverFuncs( GLcontext *ctx );
void s3vSetSpanFunctions(driRenderbuffer *rb, const GLvisual *vis);
void s3vInitState( s3vContextPtr vmesa );
void s3vInitHW( s3vContextPtr vmesa );
void s3vInitStateFuncs( GLcontext *ctx );
void s3vInitTextureFuncs( GLcontext *ctx );
void s3vInitTriFuncs( GLcontext *ctx );

void s3vUpdateWindow( GLcontext *ctx );
void s3vUpdateViewportOffset( GLcontext *ctx );

void s3vPrintLocalLRU( s3vContextPtr vmesa );
void s3vPrintGlobalLRU( s3vContextPtr vmesa );

extern void s3vFallback( s3vContextPtr vmesa, GLuint bit, GLboolean mode );
#define FALLBACK( imesa, bit, mode ) s3vFallback( imesa, bit, mode )

/* Use the templated vertex formats.  Only one of these is used in s3v.
 */
#define TAG(x) s3v##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

typedef void (*s3v_quad_func)( s3vContextPtr, 
				const s3vVertex *, 
				const s3vVertex *,
				const s3vVertex *,
				const s3vVertex * );
typedef void (*s3v_tri_func)( s3vContextPtr, 
				const s3vVertex *, 
				const s3vVertex *,
				const s3vVertex * );
typedef void (*s3v_line_func)( s3vContextPtr, 
				const s3vVertex *, 
				const s3vVertex * );
typedef void (*s3v_point_func)( s3vContextPtr, 
				 const s3vVertex * );


/* static void s3v_lines_emit(GLcontext *ctx, GLuint start, GLuint end); */
typedef void (*emit_func)( GLcontext *, GLuint, GLuint);

struct s3v_context {
	GLcontext 		*glCtx;		/* Mesa context */

	__DRIcontextPrivate	*driContext;
	__DRIscreenPrivate	*driScreen;
	__DRIdrawablePrivate	*driDrawable;

	GLuint new_gl_state;
	GLuint new_state;
	GLuint dirty;

	S3VSAREAPtr	sarea; 

	/* Temporaries for translating away float colors
	 */
	struct gl_client_array UbyteColor;
	struct gl_client_array UbyteSecondaryColor;

   	/* Mirrors of some DRI state
    	 */

	drm_context_t hHWContext;
	drmLock *driHwLock;
	int driFd;

	GLuint numClipRects;		/* Cliprects for the draw buffer */
	drm_clip_rect_t *pClipRects;

	GLuint*	buf;			/* FIXME */
	GLuint*	_buf[2];
	int		_bufNum;
	int		bufIndex[2];
	int		bufSize;
	int		bufCount;

	s3vScreenPtr 	s3vScreen;		/* Screen private DRI data */

	int		drawOffset;
	int		readOffset;

	s3v_point_func	draw_point;
	s3v_line_func	draw_line;
	s3v_tri_func	draw_tri;
	s3v_quad_func	draw_quad;

	GLuint Fallback;
	GLuint RenderIndex;
	GLuint SetupNewInputs;
	GLuint SetupIndex;

	GLuint vertex_format;
	GLuint vertex_size;
	GLuint vertex_stride_shift;
	char *verts;

	GLfloat hw_viewport[16];
	GLuint hw_primitive;
	GLenum render_primitive;

	GLfloat	depth_scale;

	s3vTextureObjectPtr CurrentTexObj[2];
	struct s3v_texture_object_t TexObjList;
	struct s3v_texture_object_t SwappedOut; 
	GLenum TexEnvImageFmt[2];

	struct mem_block *texHeap;

   	int lastSwap;
   	int texAge;
   	int ctxAge;
	int dirtyAge;
	int lastStamp;

	/* max was here: don't touch */
   
	unsigned int S3V_REG[S3V_REGS_NUM];

	GLuint texMode;
	GLuint alphaMode;
	GLuint lightMode;

	GLuint SrcBase;
	GLuint DestBase;
	GLuint DestBlit;
	GLuint ScissorLR;
	GLuint ScissorTB;
	GLuint ScissorWH; /* SubScissorWH */ /* RectWH */
	GLuint FrontStride;
	GLuint BackStride;
	GLuint SrcStride;
	GLuint DestStride;
	GLuint SrcXY;
	GLuint DestXY;

   	GLuint ClearColor;
	GLuint Color;
	GLuint DitherMode;
   	GLuint ClearDepth;

	GLuint TextureBorderColor;
	GLuint TexOffset;
	GLuint TexStride;

	GLuint CMD;
	GLuint prim_cmd;
	GLuint _tri[2]; /* 0 = gouraud; 1 = tex (lit or unlit) */
	GLuint alpha_cmd; /* actual alpha cmd */
	GLuint _alpha[2];
	GLuint _alpha_tex; /* tex alpha type */
	/* (3d_mode) 0 = 3d line/gourad tri; 1 = 3d tex tri */
	GLuint _3d_mode;
	
	GLfloat backface_sign;
	GLfloat cull_zero;

	int restore_primitive;

/* *** 2check *** */

	GLuint		FogMode;
	GLuint		AreaStippleMode;
	GLuint		LBReadFormat;
	GLuint		LBWriteFormat;
	GLuint		LineMode;
	GLuint		PointMode;
	GLuint		TriangleMode;
	GLuint		AntialiasMode;
	GLfloat		ViewportScaleX;
	GLfloat		ViewportScaleY;
	GLfloat		ViewportScaleZ;
	GLfloat		ViewportOffsetX;
	GLfloat		ViewportOffsetY;
	GLfloat		ViewportOffsetZ;
	int		MatrixMode;
	int		DepthMode;
	int		TransformMode;
	int		LBReadMode;
	int		FBReadMode;
	int		FBWindowBase;
	int		LBWindowBase;
	int		ColorDDAMode;
	int		GeometryMode;
	int		AlphaTestMode;
	int		AlphaBlendMode;
	int		AB_FBReadMode;
	int		AB_FBReadMode_Save;
	int		DeltaMode;
	int		ColorMaterialMode;
	int		FBHardwareWriteMask;
	int		MaterialMode;
	int		NormalizeMode;
	int		LightingMode;
	int		Light0Mode;
	int		Light1Mode;
	int		Light2Mode;
	int		Light3Mode;
	int		Light4Mode;
	int		Light5Mode;
	int		Light6Mode;
	int		Light7Mode;
	int		Light8Mode;
	int		Light9Mode;
	int		Light10Mode;
	int		Light11Mode;
	int		Light12Mode;
	int		Light13Mode;
	int		Light14Mode;
	int		Light15Mode;
	int		LogicalOpMode;
	int		ScissorMode;
	int		ScissorMaxXY;
	int		ScissorMinXY;
	int		Window; /* GID part probably should be in draw priv */
	int		WindowOrigin;
	int		x, y, w, h; /* Probably should be in drawable priv */
	int		FrameCount; /* Probably should be in drawable priv */
	int		NotClipped; /* Probably should be in drawable priv */
	int		WindowChanged; /* Probably should be in drawabl... */
	int		Flags;
	int		EnabledFlags;
	int		DepthSize;
	int		Begin;
	GLenum		ErrorValue;
	int		Texture1DEnabled;
	int		Texture2DEnabled;

	float		ModelView[16];
	float		Proj[16];
	float		ModelViewProj[16];
	float		Texture[16];

	float		ModelViewStack[(MAX_MODELVIEW_STACK-1)*16];
	int		ModelViewCount;
	float		ProjStack[(MAX_PROJECTION_STACK-1)*16];
	int		ProjCount;
	float		TextureStack[(MAX_TEXTURE_STACK-1)*16];
	int		TextureCount;
};

#define S3VIRGEPACKCOLOR555( r, g, b, a ) \
    ((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) | \
     ((a) ? 0x8000 : 0))

#define S3VIRGEPACKCOLOR565( r, g, b ) \
    ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

#define S3VIRGEPACKCOLOR888( r, g, b ) \
    (((r) << 16) | ((g) << 8) | (b))

#define S3VIRGEPACKCOLOR8888( r, g, b, a ) \
    (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define S3VIRGEPACKCOLOR4444( r, g, b, a ) \
    ((((a) & 0xf0) << 8) | (((r) & 0xf0) << 4) | ((g) & 0xf0) | ((b) >> 4))

static __inline GLuint s3vPackColor( GLuint cpp,
					GLubyte r, GLubyte g,
					GLubyte b, GLubyte a )
{
	unsigned int ret;
	DEBUG(("cpp = %i, r=0x%x, g=0x%x, b=0x%x, a=0x%x\n", cpp, r, g, b, a));

	switch ( cpp ) {
	case 2:
		ret = S3VIRGEPACKCOLOR555( r, g, b, a );
		DEBUG(("ret = 0x%x\n", ret));
		return ret;
	case 4:
		return PACK_COLOR_8888( a, r, g, b );
	default:
    	return 0;
	}
}

#define S3V_CONTEXT(ctx)	((s3vContextPtr)(ctx->DriverCtx))

#endif /* _S3V_CONTEXT_H_ */
