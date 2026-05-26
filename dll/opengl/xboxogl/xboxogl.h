

#pragma once

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winreg.h>
#include <GL/gl.h>

/* OpenGL 1.2 / later enums the ReactOS 1.1 GL/gl.h may not define. */
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE        0x812F
#endif
#ifndef GL_MIRRORED_REPEAT
#define GL_MIRRORED_REPEAT      0x8370
#endif
#ifndef GL_BGR
#define GL_BGR                  0x80E0
#endif
#ifndef GL_BGRA
#define GL_BGRA                 0x80E1
#endif
#ifndef GL_BGR_EXT
#define GL_BGR_EXT              0x80E0
#endif
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT             0x80E1
#endif
#ifndef GL_RESCALE_NORMAL
#define GL_RESCALE_NORMAL       0x803A
#endif
#ifndef GL_TEXTURE_3D
#define GL_TEXTURE_3D           0x806F
#endif
#ifndef GL_LIGHT_MODEL_COLOR_CONTROL
#define GL_LIGHT_MODEL_COLOR_CONTROL 0x81F8
#endif
#ifndef GL_TEXTURE_BASE_LEVEL
#define GL_TEXTURE_BASE_LEVEL   0x813C
#define GL_TEXTURE_MAX_LEVEL    0x813D
#define GL_TEXTURE_MIN_LOD      0x813A
#define GL_TEXTURE_MAX_LOD      0x813B
#endif
#ifndef GL_NORMALIZE
#define GL_NORMALIZE            0x0BA1
#endif
#ifndef GL_POLYGON_OFFSET_POINT
#define GL_POLYGON_OFFSET_POINT 0x2A01
#define GL_POLYGON_OFFSET_LINE  0x2A02
#define GL_POLYGON_OFFSET_FILL  0x8037
#endif
#ifndef GL_SINGLE_COLOR
#define GL_SINGLE_COLOR             0x81F9
#define GL_SEPARATE_SPECULAR_COLOR  0x81FA
#endif
#ifndef GL_MAX_ELEMENTS_VERTICES
#define GL_MAX_ELEMENTS_VERTICES 0x80E8
#define GL_MAX_ELEMENTS_INDICES  0x80E9
#endif
#ifndef GL_MAX_3D_TEXTURE_SIZE
#define GL_MAX_3D_TEXTURE_SIZE   0x8073
#endif
#ifndef GL_ALIASED_POINT_SIZE_RANGE
#define GL_ALIASED_POINT_SIZE_RANGE 0x846D
#define GL_SMOOTH_POINT_SIZE_RANGE  0x0B12
#define GL_ALIASED_LINE_WIDTH_RANGE 0x846E
#define GL_SMOOTH_LINE_WIDTH_RANGE  0x0B22
#endif
/* GL 1.2 blend (ARB_imaging subset) */
#ifndef GL_CONSTANT_COLOR
#define GL_CONSTANT_COLOR           0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_CONSTANT_ALPHA           0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#define GL_BLEND_COLOR              0x8005
#endif
#ifndef GL_FUNC_ADD
#define GL_FUNC_ADD                 0x8006
#define GL_MIN                      0x8007
#define GL_MAX                      0x8008
#define GL_BLEND_EQUATION           0x8009
#define GL_FUNC_SUBTRACT            0x800A
#define GL_FUNC_REVERSE_SUBTRACT    0x800B
#endif
/* GL 1.2 packed pixel transfer types */
#ifndef GL_UNSIGNED_BYTE_3_3_2
#define GL_UNSIGNED_BYTE_3_3_2      0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4   0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1   0x8034
#define GL_UNSIGNED_INT_8_8_8_8     0x8035
#define GL_UNSIGNED_INT_10_10_10_2  0x8036
#define GL_UNSIGNED_BYTE_2_3_3_REV  0x8362
#define GL_UNSIGNED_SHORT_5_6_5     0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV 0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#endif

/* Bring in the DHGLRC + GLCLTPROCTABLE definitions from opengl32.dll's ICD
 * contract.  We #include the same icd.h opengl32 uses; everything we need is
 * the type definitions, not the linker-visible state. */
#include "../opengl32/icd.h"

/* Shared NV2A IOCTL surface + the private DrvEscape code for the 3D triangle
 * path (NV2A_DRAW_3D, XBOX_ESC_NV2A_DRAW3D). */
#include "../../../win32ss/drivers/miniport/xboxvmp/nv2a_accel.h"

/* Matrix stack depth (modelview + projection, GL 1.x guarantees 32) */
#define XGL_MATRIX_STACK_DEPTH  32

typedef struct _XGL_MAT4
{
    float m[16]; /* column-major, OpenGL convention */
} XGL_MAT4;

typedef enum _XGL_MAT_MODE
{
    XGL_MAT_MODELVIEW = 0,
    XGL_MAT_PROJECTION = 1,
    XGL_MAT_TEXTURE   = 2,
    XGL_MAT_MODE_MAX
} XGL_MAT_MODE;

/* An immediate-mode vertex: object-space position + per-vertex attributes as
 * supplied between glBegin/glEnd.  Transform happens at glEnd. */
typedef struct _XGL_VERTEX
{
    float x, y, z, w;       /* object-space position (w defaults to 1) */
    float r, g, b, a;       /* colour, [0,1] */
    float nx, ny, nz;       /* normal */
    float s, t;             /* texture coord 0 */
} XGL_VERTEX;

#define XGL_VERTEX_BUFFER_CAP  1024

/* GL 1.1 texture object.  Games like Unreal Tournament allocate far more than
 * 256 texture names; once glGenTextures handed out a name >= the cap it silently
 * returned 0 and the texture vanished (rendered white).  4096 covers real games;
 * the array lives in the heap-allocated context (~52 bytes each ≈ 213 KB). */
#define XGL_MAX_TEXTURES 4096
typedef struct _XGL_TEXTURE
{
    GLuint   name;
    BOOL     used;
    int      width, height;
    GLenum   minFilter, magFilter;
    GLenum   wrapS, wrapT;
    GLenum   envMode;       /* GL_MODULATE / GL_REPLACE / GL_DECAL */
    DWORD   *texels;        /* width*height ARGB (0 if none) */
    ULONG    gpuOffset;     /* VRAM offset of the uploaded texture (0 = not uploaded) */
} XGL_TEXTURE;

/* Client (vertex array) state for one attribute. */
typedef struct _XGL_ARRAY
{
    BOOL          enabled;
    GLint         size;
    GLenum        type;
    GLsizei       stride;
    const GLvoid *ptr;
} XGL_ARRAY;

/* GL 1.1 display list.  We record the immediate-mode call stream (object-space
 * geometry + attribute changes) as a flat token buffer and replay it through the
 * same xgl_real_* entry points at glCallList time, so the list is transformed by
 * whatever matrix is current when it's called (exactly GL display-list semantics).
 * Voxel games on the GL 1.1 path emulate one vertex buffer per world chunk as a
 * display list (e.g. ClassiCube's GL11 backend bakes glDrawElements into a list),
 * so the world allocates hundreds-to-thousands of lists at once — 256 starved it. */
#define XGL_MAX_LISTS 8192
typedef struct _XGL_DLIST
{
    BOOL   used;
    float *data;     /* token stream (opcode + args, all stored as float) */
    int    count;    /* floats used */
    int    cap;      /* floats allocated */
} XGL_DLIST;

/* GL 1.1 fixed-function light (software T&L on the CPU; feeds the GPU rasteriser). */
#define XGL_MAX_LIGHTS 8
typedef struct _XGL_LIGHT
{
    BOOL  enabled;
    float pos[4];        /* EYE space (transformed by the modelview at glLight time); w=0 directional */
    float ambient[4];
    float diffuse[4];
    float specular[4];
} XGL_LIGHT;

/* glPushAttrib/glPopAttrib snapshot (the commonly-pushed enable + render state;
 * matrices/textures/lighting-material are not snapshotted). */
#define XGL_ATTRIB_STACK 16
typedef struct _XGL_ATTRIB
{
    BOOL      enDepthTest, enBlend, enCullFace, enTexture2D, enScissor, enLighting, enFog, enAlphaTest;
    GLenum    depthFunc;  GLboolean depthMask;
    GLenum    shadeModel, cullFaceMode, frontFace, blendSrc, blendDst, polygonMode, alphaFunc;
    float     alphaRef, pointSize, lineWidth;
    float     curColor[4];
} XGL_ATTRIB;

/* Remaining GL 1.1 feature state (texgen, clip planes, selection/feedback,
 * evaluators, raster/pixel ops). */
#define XGL_MAX_CLIP_PLANES   6
#define XGL_NAME_STACK_DEPTH  64
#define XGL_EVAL_TARGETS      9    /* the 9 GL_MAP* targets (see EvalTargetIndex) */

/* One evaluator control mesh (glMap1/glMap2).  vorder==1 for a 1D map. */
typedef struct _XGL_EVALMAP
{
    BOOL   defined;
    int    components;        /* values per control point (1..4) */
    int    uorder, vorder;    /* control points along u / v */
    float  u1, u2, v1, v2;    /* parameter domain */
    float *points;            /* components*uorder*vorder floats (heap) */
} XGL_EVALMAP;

/* glPushClientAttrib/glPopClientAttrib snapshot (vertex-array + pixel-store). */
typedef struct _XGL_CLIENT_ATTRIB
{
    struct _XGL_ARRAY av, ac, an, at;   /* vertex/colour/normal/texcoord arrays */
    GLint  unpackAlignment, unpackRowLength, unpackSkipPixels, unpackSkipRows, packAlignment;
} XGL_CLIENT_ATTRIB;

typedef struct _XGL_CONTEXT
{
    HDC      hdc;
    UINT     width;
    UINT     height;

    /* Frame control for the NV2A 3D engine.  Geometry is rendered by the GPU into
     * a persistent offscreen surface in the miniport; the ICD clears it once per
     * frame, submits triangle batches, then presents.  glClear records a pending
     * clear that rides with the next batch (or the present) via NV2A_DRAW3D_FLAG_*. */
    BOOL     clearPending;
    BOOL     clearDepthPending; /* glClear(GL_DEPTH_BUFFER_BIT) pending for the next batch */
    BOOL     clearStencilPending; /* glClear(GL_STENCIL_BUFFER_BIT) pending (Z24S8 only) */
    ULONG    clearPacked;    /* 0x00RRGGBB clear colour */

    /* Clear state */
    float    clearColor[4];  /* RGBA, [0,1] */
    float    clearDepth;     /* [0,1] */

    /* Current immediate-mode attributes */
    float    curColor[4];
    float    curNormal[3];
    float    curTexCoord[4];

    /* Begin/End */
    GLenum   primitive;          /* 0 = not inside Begin */
    XGL_VERTEX verts[XGL_VERTEX_BUFFER_CAP];
    UINT     vertCount;

    /* Matrices */
    XGL_MAT_MODE matMode;
    XGL_MAT4   matStack[XGL_MAT_MODE_MAX][XGL_MATRIX_STACK_DEPTH];
    UINT       matTop[XGL_MAT_MODE_MAX];

    /* Viewport + depth range */
    GLint    vpX, vpY;
    GLsizei  vpW, vpH;
    float    depthNear, depthFar;

    /* Capabilities / state */
    BOOL     enDepthTest;
    BOOL     enBlend;
    BOOL     enCullFace;
    BOOL     enTexture2D;
    BOOL     enScissor;
    BOOL      enLighting;            /* GL_LIGHTING */
    XGL_LIGHT lights[XGL_MAX_LIGHTS];
    float     globalAmbient[4];      /* GL_LIGHT_MODEL_AMBIENT */
    float     matAmbient[4];
    float     matDiffuse[4];
    float     matEmission[4];
    float     matSpecular[4];
    float     matShininess;

    /* Point size / line width (line width is informational: NV2A 3D lines are 1px) */
    float     pointSize, lineWidth;
    int       swapInterval;          /* wglSwapIntervalEXT (WGL_EXT_swap_control); tracked only */

    /* glPushAttrib/glPopAttrib */
    XGL_ATTRIB attribStack[XGL_ATTRIB_STACK];
    UINT       attribTop;

    /* Fog (GL_FOG) — blended into the vertex colour during CPU T&L */
    BOOL      enFog;
    GLenum    fogMode;               /* GL_LINEAR / GL_EXP / GL_EXP2 */
    float     fogColor[4];
    float     fogStart, fogEnd, fogDensity;

    /* Alpha test + polygon mode (GPU state) */
    BOOL      enAlphaTest;
    GLenum    alphaFunc;             /* GL_NEVER..GL_ALWAYS (0x200..0x207) */
    float     alphaRef;              /* [0,1] */
    GLenum    polygonMode;           /* GL_POINT/LINE/FILL (== NV2A values) */
    BOOL      enPolyOffsetFill, enPolyOffsetLine, enPolyOffsetPoint; /* glEnable(GL_POLYGON_OFFSET_*) */
    float     polyOffsetFactor, polyOffsetUnits;                     /* glPolygonOffset */
    BOOL      enColorMaterial;       /* GL_COLOR_MATERIAL: glColor tracks the material */
    GLenum    colorMaterialMode;     /* GL_AMBIENT/DIFFUSE/AMBIENT_AND_DIFFUSE/SPECULAR/EMISSION */
    GLenum   depthFunc;
    GLboolean depthMask;
    GLenum   shadeModel;     /* GL_FLAT / GL_SMOOTH */
    GLenum   cullFaceMode;   /* GL_BACK / GL_FRONT / GL_FRONT_AND_BACK */
    GLenum   frontFace;      /* GL_CCW / GL_CW */
    GLenum   blendSrc, blendDst;
    GLenum   blendEquation;          /* GL 1.2: GL_FUNC_ADD/SUBTRACT/REVERSE_SUBTRACT/MIN/MAX */
    float    blendColor[4];          /* GL 1.2: glBlendColor constant */
    GLenum   colorControl;           /* GL 1.2: GL_SINGLE_COLOR / GL_SEPARATE_SPECULAR_COLOR */
    BOOL     enRescaleNormal;        /* GL 1.2 GL_RESCALE_NORMAL / GL_NORMALIZE (we always normalise) */
    GLint    scissorX, scissorY, scissorW, scissorH;
    GLenum   matrixMode;
    GLenum   lastError;

    /* Textures */
    XGL_TEXTURE textures[XGL_MAX_TEXTURES];
    GLuint   boundTexture;
    GLuint   nextTexture;
    float    texEnvColor[4];   /* GL_TEXTURE_ENV_COLOR (for GL_BLEND texenv) */

    /* Vertex arrays (GL 1.1) */
    XGL_ARRAY arrVertex, arrColor, arrNormal, arrTexCoord;

    /* Display lists (GL 1.1) */
    XGL_DLIST lists[XGL_MAX_LISTS];
    GLuint    listRecording;   /* list currently being compiled (0 = none) */
    GLenum    listMode;        /* GL_COMPILE or GL_COMPILE_AND_EXECUTE */
    GLuint    listBase;        /* glListBase: offset added to glCallLists indices */

    /* Texture coordinate generation (glTexGen), per coord S,T,R,Q (index 0..3). */
    BOOL      texGenEnabled[4];
    GLenum    texGenMode[4];           /* GL_OBJECT_LINEAR / GL_EYE_LINEAR / GL_SPHERE_MAP */
    float     texGenObjPlane[4][4];
    float     texGenEyePlane[4][4];    /* stored as specified (assumed under current modelview) */

    /* User clip planes (glClipPlane / GL_CLIP_PLANEi), stored in eye space. */
    BOOL      clipPlaneEnabled[XGL_MAX_CLIP_PLANES];
    double    clipPlane[XGL_MAX_CLIP_PLANES][4];

    /* Raster position + pixel store/zoom (glRasterPos/glBitmap/glDrawPixels). */
    float     rasterPos[4];            /* window pixel x,y,z,w after transform */
    BOOL      rasterValid;
    float     rasterColor[4];
    GLint     unpackAlignment, unpackRowLength, unpackSkipPixels, unpackSkipRows;
    GLint     packAlignment;
    float     pixelZoomX, pixelZoomY;

    /* glColorMask (tracked; the HW write-mask would need a miniport register). */
    GLboolean colorMask[4];
    /* glLineStipple / glPolygonStipple (tracked; the NV2A 3D path we drive doesn't stipple). */
    BOOL      enLineStipple, enPolygonStipple;
    GLint     lineStippleFactor;
    GLushort  lineStipplePattern;
    GLubyte   polygonStipple[128];
    /* glEdgeFlag (tracked). */
    GLboolean edgeFlag;
    /* glLogicOp / glDrawBuffer / glReadBuffer (tracked). */
    BOOL      enColorLogicOp;
    GLenum    logicOp, drawBuffer, readBuffer;
    /* Accumulation buffer: a CPU float buffer (RGBA per pixel, top-down) realised
     * via the offscreen readback/writeback escapes.  Lazily allocated. */
    float     accumClear[4];
    float    *accumBuf;
    GLint     accumW, accumH;
    /* Colour-index mode: RGBA-only visual — tracked. */
    float     curIndex, clearIndexVal;
    GLuint    indexWriteMask;
    /* Stencil: Z16 zeta has no stencil bits — tracked for honest queries, render no-op. */
    BOOL      enStencilTest;
    GLint     clearStencil, stencilRef;
    GLuint    stencilWriteMask, stencilValueMask;
    GLenum    stencilFunc, stencilFail, stencilZFail, stencilZPass;

    /* Selection (glSelectBuffer / glRenderMode GL_SELECT) + name stack. */
    GLenum    renderMode;
    GLuint   *selectBuffer; GLsizei selectBufferSize, selectIndex;
    GLuint    nameStack[XGL_NAME_STACK_DEPTH]; GLint nameStackDepth;
    BOOL      selHitActive; GLuint selHitNames; float selHitZMin, selHitZMax;
    GLboolean selOverflow;
    GLint     selHitCount;
    /* Feedback (glFeedbackBuffer / glRenderMode GL_FEEDBACK). */
    GLfloat  *feedbackBuffer; GLsizei feedbackBufferSize, feedbackIndex;
    GLenum    feedbackType; GLboolean feedbackOverflow;

    /* Evaluators (glMap1/glMap2 + grids), indexed by EvalTargetIndex(). */
    XGL_EVALMAP evalMap1[XGL_EVAL_TARGETS];
    XGL_EVALMAP evalMap2[XGL_EVAL_TARGETS];
    BOOL      evalMap1Enabled[XGL_EVAL_TARGETS], evalMap2Enabled[XGL_EVAL_TARGETS];
    BOOL      enAutoNormal;
    GLint     mapGrid1n; float mapGrid1u1, mapGrid1u2;
    GLint     mapGrid2un, mapGrid2vn; float mapGrid2u1, mapGrid2u2, mapGrid2v1, mapGrid2v2;

    /* glPushClientAttrib/glPopClientAttrib stack. */
    XGL_CLIENT_ATTRIB clientAttribStack[XGL_ATTRIB_STACK];
    UINT      clientAttribTop;
} XGL_CONTEXT, *PXGL_CONTEXT;

/* dispatch.c */
extern GLCLTPROCTABLE g_xglDispatchTable;
void XglDispatchInstallReal(void);

/* real.c */
PXGL_CONTEXT XglCurrent(void);
void XglSetCurrent(PXGL_CONTEXT ctx);
void XglMatIdentity(XGL_MAT4 *m);
void XglMatMul(XGL_MAT4 *out, const XGL_MAT4 *a, const XGL_MAT4 *b);
void XglMatTransform(const XGL_MAT4 *m, const float in[4], float out[4]);

/* Present the current context's frame to the window (icd.c DrvSwapBuffers). */
void XglPresentCurrent(void);

/* Real GL function decls (overrides for the stub table) */
void GLAPIENTRY xgl_real_Clear(GLbitfield mask);
void GLAPIENTRY xgl_real_ClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
void GLAPIENTRY xgl_real_ClearDepth(GLclampd d);

void GLAPIENTRY xgl_real_Color4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void GLAPIENTRY xgl_real_Color3f(GLfloat r, GLfloat g, GLfloat b);
void GLAPIENTRY xgl_real_Color4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a);
void GLAPIENTRY xgl_real_Color3ub(GLubyte r, GLubyte g, GLubyte b);
void GLAPIENTRY xgl_real_Color4fv(const GLfloat *v);
void GLAPIENTRY xgl_real_Color3fv(const GLfloat *v);
void GLAPIENTRY xgl_real_Color3ubv(const GLubyte *v);
void GLAPIENTRY xgl_real_Color4ubv(const GLubyte *v);
void GLAPIENTRY xgl_real_Color3d(GLdouble r, GLdouble g, GLdouble b);
void GLAPIENTRY xgl_real_Normal3f(GLfloat x, GLfloat y, GLfloat z);
void GLAPIENTRY xgl_real_Normal3fv(const GLfloat *v);
void GLAPIENTRY xgl_real_TexCoord2f(GLfloat s, GLfloat t);
void GLAPIENTRY xgl_real_TexCoord2fv(const GLfloat *v);
void GLAPIENTRY xgl_real_TexCoord2i(GLint s, GLint t);

void GLAPIENTRY xgl_real_Begin(GLenum mode);
void GLAPIENTRY xgl_real_End(void);
void GLAPIENTRY xgl_real_Vertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void GLAPIENTRY xgl_real_Vertex3f(GLfloat x, GLfloat y, GLfloat z);
void GLAPIENTRY xgl_real_Vertex2f(GLfloat x, GLfloat y);
void GLAPIENTRY xgl_real_Vertex2i(GLint x, GLint y);
void GLAPIENTRY xgl_real_Vertex3i(GLint x, GLint y, GLint z);
void GLAPIENTRY xgl_real_Vertex3fv(const GLfloat *v);
void GLAPIENTRY xgl_real_Vertex2fv(const GLfloat *v);

/* Type-variant entry points */
void GLAPIENTRY xgl_real_Vertex2d(GLdouble x, GLdouble y);
void GLAPIENTRY xgl_real_Vertex3d(GLdouble x, GLdouble y, GLdouble z);
void GLAPIENTRY xgl_real_Vertex2s(GLshort x, GLshort y);
void GLAPIENTRY xgl_real_Vertex3s(GLshort x, GLshort y, GLshort z);
void GLAPIENTRY xgl_real_Vertex4i(GLint x, GLint y, GLint z, GLint w);
void GLAPIENTRY xgl_real_Vertex2dv(const GLdouble *v);
void GLAPIENTRY xgl_real_Vertex3dv(const GLdouble *v);
void GLAPIENTRY xgl_real_Vertex4fv(const GLfloat *v);
void GLAPIENTRY xgl_real_Color4d(GLdouble r, GLdouble g, GLdouble b, GLdouble a);
void GLAPIENTRY xgl_real_Color3b(GLbyte r, GLbyte g, GLbyte b);
void GLAPIENTRY xgl_real_Color4b(GLbyte r, GLbyte g, GLbyte b, GLbyte a);
void GLAPIENTRY xgl_real_Color4dv(const GLdouble *v);
void GLAPIENTRY xgl_real_Color3dv(const GLdouble *v);
void GLAPIENTRY xgl_real_Normal3d(GLdouble x, GLdouble y, GLdouble z);
void GLAPIENTRY xgl_real_Normal3b(GLbyte x, GLbyte y, GLbyte z);
void GLAPIENTRY xgl_real_Normal3dv(const GLdouble *v);
void GLAPIENTRY xgl_real_TexCoord1f(GLfloat s);
void GLAPIENTRY xgl_real_TexCoord3f(GLfloat s, GLfloat t, GLfloat r);
void GLAPIENTRY xgl_real_TexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void GLAPIENTRY xgl_real_TexCoord2d(GLdouble s, GLdouble t);
void GLAPIENTRY xgl_real_TexCoord1d(GLdouble s);
void GLAPIENTRY xgl_real_TexCoord3fv(const GLfloat *v);
void GLAPIENTRY xgl_real_TexCoord4fv(const GLfloat *v);
void GLAPIENTRY xgl_real_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void GLAPIENTRY xgl_real_Recti(GLint x1, GLint y1, GLint x2, GLint y2);
void GLAPIENTRY xgl_real_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
void GLAPIENTRY xgl_real_Rectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void GLAPIENTRY xgl_real_PointSize(GLfloat size);
void GLAPIENTRY xgl_real_LineWidth(GLfloat width);
void GLAPIENTRY xgl_real_PushAttrib(GLbitfield mask);
void GLAPIENTRY xgl_real_PopAttrib(void);

void GLAPIENTRY xgl_real_VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *p);
void GLAPIENTRY xgl_real_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *p);
void GLAPIENTRY xgl_real_NormalPointer(GLenum type, GLsizei stride, const GLvoid *p);
void GLAPIENTRY xgl_real_TexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *p);
void GLAPIENTRY xgl_real_EnableClientState(GLenum a);
void GLAPIENTRY xgl_real_DisableClientState(GLenum a);
void GLAPIENTRY xgl_real_DrawArrays(GLenum mode, GLint first, GLsizei count);
void GLAPIENTRY xgl_real_DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void GLAPIENTRY xgl_real_DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

void GLAPIENTRY xgl_real_MatrixMode(GLenum mode);
void GLAPIENTRY xgl_real_LoadIdentity(void);
void GLAPIENTRY xgl_real_LoadMatrixf(const GLfloat *m);
void GLAPIENTRY xgl_real_MultMatrixf(const GLfloat *m);
void GLAPIENTRY xgl_real_LoadMatrixd(const GLdouble *m);
void GLAPIENTRY xgl_real_MultMatrixd(const GLdouble *m);
void GLAPIENTRY xgl_real_PushMatrix(void);
void GLAPIENTRY xgl_real_PopMatrix(void);
void GLAPIENTRY xgl_real_Ortho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f);
void GLAPIENTRY xgl_real_Frustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f);
void GLAPIENTRY xgl_real_Translatef(GLfloat x, GLfloat y, GLfloat z);
void GLAPIENTRY xgl_real_Scalef(GLfloat x, GLfloat y, GLfloat z);
void GLAPIENTRY xgl_real_Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void GLAPIENTRY xgl_real_Translated(GLdouble x, GLdouble y, GLdouble z);
void GLAPIENTRY xgl_real_Scaled(GLdouble x, GLdouble y, GLdouble z);
void GLAPIENTRY xgl_real_Rotated(GLdouble a, GLdouble x, GLdouble y, GLdouble z);

void GLAPIENTRY xgl_real_Viewport(GLint x, GLint y, GLsizei w, GLsizei h);
void GLAPIENTRY xgl_real_DepthRange(GLclampd n, GLclampd f);
void GLAPIENTRY xgl_real_Scissor(GLint x, GLint y, GLsizei w, GLsizei h);

void GLAPIENTRY xgl_real_Enable(GLenum cap);
void GLAPIENTRY xgl_real_Disable(GLenum cap);
GLboolean GLAPIENTRY xgl_real_IsEnabled(GLenum cap);
void GLAPIENTRY xgl_real_DepthFunc(GLenum f);
void GLAPIENTRY xgl_real_DepthMask(GLboolean m);
void GLAPIENTRY xgl_real_ShadeModel(GLenum m);
void GLAPIENTRY xgl_real_CullFace(GLenum m);
void GLAPIENTRY xgl_real_FrontFace(GLenum m);
void GLAPIENTRY xgl_real_BlendFunc(GLenum s, GLenum d);
void GLAPIENTRY xgl_real_Hint(GLenum target, GLenum mode);

void GLAPIENTRY xgl_real_GenTextures(GLsizei n, GLuint *out);
void GLAPIENTRY xgl_real_DeleteTextures(GLsizei n, const GLuint *names);
void GLAPIENTRY xgl_real_BindTexture(GLenum target, GLuint name);
void GLAPIENTRY xgl_real_TexImage2D(GLenum target, GLint level, GLint internalFormat,
                                    GLsizei w, GLsizei h, GLint border, GLenum format,
                                    GLenum type, const GLvoid *pixels);
void GLAPIENTRY xgl_real_TexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff,
                                       GLsizei w, GLsizei h, GLenum format, GLenum type,
                                       const GLvoid *pixels);
void GLAPIENTRY xgl_real_TexParameteri(GLenum target, GLenum pname, GLint param);
void GLAPIENTRY xgl_real_TexParameterf(GLenum t, GLenum p, GLfloat v);
void GLAPIENTRY xgl_real_TexEnvi(GLenum target, GLenum pname, GLint param);
void GLAPIENTRY xgl_real_TexEnvf(GLenum t, GLenum p, GLfloat v);

GLenum GLAPIENTRY xgl_real_GetError(void);
void GLAPIENTRY xgl_real_GetIntegerv(GLenum pname, GLint *p);
void GLAPIENTRY xgl_real_GetFloatv(GLenum pname, GLfloat *p);
const GLubyte * GLAPIENTRY xgl_real_GetString(GLenum name);

void GLAPIENTRY xgl_real_Finish(void);
void GLAPIENTRY xgl_real_Flush(void);

/* Display lists (GL 1.1) */
GLuint GLAPIENTRY xgl_real_GenLists(GLsizei range);
void GLAPIENTRY xgl_real_NewList(GLuint list, GLenum mode);
void GLAPIENTRY xgl_real_EndList(void);
void GLAPIENTRY xgl_real_CallList(GLuint list);
void GLAPIENTRY xgl_real_DeleteLists(GLuint list, GLsizei range);
GLboolean GLAPIENTRY xgl_real_IsList(GLuint list);

/* Lighting / materials (no HW lighting: materials map to the current colour) */
void GLAPIENTRY xgl_real_Materialfv(GLenum face, GLenum pname, const GLfloat *params);
void GLAPIENTRY xgl_real_Materialf(GLenum face, GLenum pname, GLfloat param);
void GLAPIENTRY xgl_real_Lightfv(GLenum light, GLenum pname, const GLfloat *params);
void GLAPIENTRY xgl_real_Lightf(GLenum light, GLenum pname, GLfloat param);
void GLAPIENTRY xgl_real_LightModelfv(GLenum pname, const GLfloat *params);
void GLAPIENTRY xgl_real_ColorMaterial(GLenum face, GLenum mode);
void GLAPIENTRY xgl_real_Fogf(GLenum pname, GLfloat param);
void GLAPIENTRY xgl_real_Fogi(GLenum pname, GLint param);
void GLAPIENTRY xgl_real_Fogfv(GLenum pname, const GLfloat *params);
void GLAPIENTRY xgl_real_AlphaFunc(GLenum func, GLclampf ref);
void GLAPIENTRY xgl_real_PolygonMode(GLenum face, GLenum mode);
void GLAPIENTRY xgl_real_PolygonOffset(GLfloat factor, GLfloat units);

/* WGL extension entry points — reached via wglGetProcAddress -> DrvGetProcAddress. */
BOOL WINAPI xgl_wglSwapIntervalEXT(int interval);
int  WINAPI xgl_wglGetSwapIntervalEXT(void);

/* GL 1.2 entry points — reached via wglGetProcAddress -> DrvGetProcAddress. */
void GLAPIENTRY xgl_real_BlendColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
void GLAPIENTRY xgl_real_BlendEquation(GLenum mode);
void GLAPIENTRY xgl_real_TexImage3D(GLenum target, GLint level, GLint internalFormat,
                                    GLsizei w, GLsizei h, GLsizei d, GLint border,
                                    GLenum format, GLenum type, const GLvoid *pixels);
void GLAPIENTRY xgl_real_TexSubImage3D(GLenum target, GLint level, GLint xoff, GLint yoff,
                                       GLint zoff, GLsizei w, GLsizei h, GLsizei d,
                                       GLenum format, GLenum type, const GLvoid *pixels);
void GLAPIENTRY xgl_real_CopyTexSubImage3D(GLenum target, GLint level, GLint xoff, GLint yoff,
                                           GLint zoff, GLint x, GLint y, GLsizei w, GLsizei h);

/* ---- Remaining GL 1.1 entry points (completed feature set) ---- */

/* Display lists */
void GLAPIENTRY xgl_real_CallLists(GLsizei n, GLenum type, const GLvoid *lists);
void GLAPIENTRY xgl_real_ListBase(GLuint base);

/* Immediate-mode type variants (funnel to the canonical f/ub forms) */
void GLAPIENTRY xgl_real_Color3bv(const GLbyte *v);
void GLAPIENTRY xgl_real_Color3i(GLint r, GLint g, GLint b);
void GLAPIENTRY xgl_real_Color3iv(const GLint *v);
void GLAPIENTRY xgl_real_Color3s(GLshort r, GLshort g, GLshort b);
void GLAPIENTRY xgl_real_Color3sv(const GLshort *v);
void GLAPIENTRY xgl_real_Color3ui(GLuint r, GLuint g, GLuint b);
void GLAPIENTRY xgl_real_Color3uiv(const GLuint *v);
void GLAPIENTRY xgl_real_Color3us(GLushort r, GLushort g, GLushort b);
void GLAPIENTRY xgl_real_Color3usv(const GLushort *v);
void GLAPIENTRY xgl_real_Color4bv(const GLbyte *v);
void GLAPIENTRY xgl_real_Color4i(GLint r, GLint g, GLint b, GLint a);
void GLAPIENTRY xgl_real_Color4iv(const GLint *v);
void GLAPIENTRY xgl_real_Color4s(GLshort r, GLshort g, GLshort b, GLshort a);
void GLAPIENTRY xgl_real_Color4sv(const GLshort *v);
void GLAPIENTRY xgl_real_Color4ui(GLuint r, GLuint g, GLuint b, GLuint a);
void GLAPIENTRY xgl_real_Color4uiv(const GLuint *v);
void GLAPIENTRY xgl_real_Color4us(GLushort r, GLushort g, GLushort b, GLushort a);
void GLAPIENTRY xgl_real_Color4usv(const GLushort *v);
void GLAPIENTRY xgl_real_Normal3bv(const GLbyte *v);
void GLAPIENTRY xgl_real_Normal3i(GLint x, GLint y, GLint z);
void GLAPIENTRY xgl_real_Normal3iv(const GLint *v);
void GLAPIENTRY xgl_real_Normal3s(GLshort x, GLshort y, GLshort z);
void GLAPIENTRY xgl_real_Normal3sv(const GLshort *v);
void GLAPIENTRY xgl_real_Vertex2iv(const GLint *v);
void GLAPIENTRY xgl_real_Vertex2sv(const GLshort *v);
void GLAPIENTRY xgl_real_Vertex3iv(const GLint *v);
void GLAPIENTRY xgl_real_Vertex3sv(const GLshort *v);
void GLAPIENTRY xgl_real_Vertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void GLAPIENTRY xgl_real_Vertex4dv(const GLdouble *v);
void GLAPIENTRY xgl_real_Vertex4iv(const GLint *v);
void GLAPIENTRY xgl_real_Vertex4s(GLshort x, GLshort y, GLshort z, GLshort w);
void GLAPIENTRY xgl_real_Vertex4sv(const GLshort *v);
void GLAPIENTRY xgl_real_TexCoord1fv(const GLfloat *v);
void GLAPIENTRY xgl_real_TexCoord1i(GLint s);
void GLAPIENTRY xgl_real_TexCoord1iv(const GLint *v);
void GLAPIENTRY xgl_real_TexCoord1s(GLshort s);
void GLAPIENTRY xgl_real_TexCoord1sv(const GLshort *v);
void GLAPIENTRY xgl_real_TexCoord1dv(const GLdouble *v);
void GLAPIENTRY xgl_real_TexCoord2dv(const GLdouble *v);
void GLAPIENTRY xgl_real_TexCoord2iv(const GLint *v);
void GLAPIENTRY xgl_real_TexCoord2s(GLshort s, GLshort t);
void GLAPIENTRY xgl_real_TexCoord2sv(const GLshort *v);
void GLAPIENTRY xgl_real_TexCoord3d(GLdouble s, GLdouble t, GLdouble r);
void GLAPIENTRY xgl_real_TexCoord3dv(const GLdouble *v);
void GLAPIENTRY xgl_real_TexCoord3i(GLint s, GLint t, GLint r);
void GLAPIENTRY xgl_real_TexCoord3iv(const GLint *v);
void GLAPIENTRY xgl_real_TexCoord3s(GLshort s, GLshort t, GLshort r);
void GLAPIENTRY xgl_real_TexCoord3sv(const GLshort *v);
void GLAPIENTRY xgl_real_TexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void GLAPIENTRY xgl_real_TexCoord4dv(const GLdouble *v);
void GLAPIENTRY xgl_real_TexCoord4i(GLint s, GLint t, GLint r, GLint q);
void GLAPIENTRY xgl_real_TexCoord4iv(const GLint *v);
void GLAPIENTRY xgl_real_TexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q);
void GLAPIENTRY xgl_real_TexCoord4sv(const GLshort *v);
void GLAPIENTRY xgl_real_Rectdv(const GLdouble *a, const GLdouble *b);
void GLAPIENTRY xgl_real_Rectfv(const GLfloat *a, const GLfloat *b);
void GLAPIENTRY xgl_real_Rectiv(const GLint *a, const GLint *b);
void GLAPIENTRY xgl_real_Rectsv(const GLshort *a, const GLshort *b);
void GLAPIENTRY xgl_real_EdgeFlag(GLboolean f);
void GLAPIENTRY xgl_real_EdgeFlagv(const GLboolean *f);

/* Colour-index mode (RGBA visual: tracked) */
void GLAPIENTRY xgl_real_Indexd(GLdouble c);
void GLAPIENTRY xgl_real_Indexdv(const GLdouble *c);
void GLAPIENTRY xgl_real_Indexf(GLfloat c);
void GLAPIENTRY xgl_real_Indexfv(const GLfloat *c);
void GLAPIENTRY xgl_real_Indexi(GLint c);
void GLAPIENTRY xgl_real_Indexiv(const GLint *c);
void GLAPIENTRY xgl_real_Indexs(GLshort c);
void GLAPIENTRY xgl_real_Indexsv(const GLshort *c);
void GLAPIENTRY xgl_real_Indexub(GLubyte c);
void GLAPIENTRY xgl_real_Indexubv(const GLubyte *c);
void GLAPIENTRY xgl_real_IndexMask(GLuint mask);
void GLAPIENTRY xgl_real_ClearIndex(GLfloat c);

/* Integer/array variants of state setters */
void GLAPIENTRY xgl_real_Fogiv(GLenum pname, const GLint *params);
void GLAPIENTRY xgl_real_Lighti(GLenum light, GLenum pname, GLint param);
void GLAPIENTRY xgl_real_Lightiv(GLenum light, GLenum pname, const GLint *params);
void GLAPIENTRY xgl_real_LightModelf(GLenum pname, GLfloat param);
void GLAPIENTRY xgl_real_LightModeli(GLenum pname, GLint param);
void GLAPIENTRY xgl_real_LightModeliv(GLenum pname, const GLint *params);
void GLAPIENTRY xgl_real_Materiali(GLenum face, GLenum pname, GLint param);
void GLAPIENTRY xgl_real_Materialiv(GLenum face, GLenum pname, const GLint *params);
void GLAPIENTRY xgl_real_TexParameterfv(GLenum t, GLenum p, const GLfloat *v);
void GLAPIENTRY xgl_real_TexParameteriv(GLenum t, GLenum p, const GLint *v);
void GLAPIENTRY xgl_real_TexEnvfv(GLenum t, GLenum p, const GLfloat *v);
void GLAPIENTRY xgl_real_TexEnviv(GLenum t, GLenum p, const GLint *v);

/* Texture coordinate generation */
void GLAPIENTRY xgl_real_TexGeni(GLenum coord, GLenum pname, GLint param);
void GLAPIENTRY xgl_real_TexGenf(GLenum coord, GLenum pname, GLfloat param);
void GLAPIENTRY xgl_real_TexGend(GLenum coord, GLenum pname, GLdouble param);
void GLAPIENTRY xgl_real_TexGeniv(GLenum coord, GLenum pname, const GLint *params);
void GLAPIENTRY xgl_real_TexGenfv(GLenum coord, GLenum pname, const GLfloat *params);
void GLAPIENTRY xgl_real_TexGendv(GLenum coord, GLenum pname, const GLdouble *params);
void GLAPIENTRY xgl_real_GetTexGeniv(GLenum coord, GLenum pname, GLint *params);
void GLAPIENTRY xgl_real_GetTexGenfv(GLenum coord, GLenum pname, GLfloat *params);
void GLAPIENTRY xgl_real_GetTexGendv(GLenum coord, GLenum pname, GLdouble *params);

/* 1D textures (degrade to a height-1 2D texture) */
void GLAPIENTRY xgl_real_TexImage1D(GLenum target, GLint level, GLint internalFormat,
                                    GLsizei w, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void GLAPIENTRY xgl_real_TexSubImage1D(GLenum target, GLint level, GLint xoff,
                                       GLsizei w, GLenum format, GLenum type, const GLvoid *pixels);

/* User clip planes */
void GLAPIENTRY xgl_real_ClipPlane(GLenum plane, const GLdouble *eqn);
void GLAPIENTRY xgl_real_GetClipPlane(GLenum plane, GLdouble *eqn);

/* Framebuffer write masks / misc state (tracked) */
void GLAPIENTRY xgl_real_ColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
void GLAPIENTRY xgl_real_LineStipple(GLint factor, GLushort pattern);
void GLAPIENTRY xgl_real_PolygonStipple(const GLubyte *mask);
void GLAPIENTRY xgl_real_GetPolygonStipple(GLubyte *mask);
void GLAPIENTRY xgl_real_LogicOp(GLenum opcode);
void GLAPIENTRY xgl_real_DrawBuffer(GLenum mode);
void GLAPIENTRY xgl_real_ReadBuffer(GLenum mode);
void GLAPIENTRY xgl_real_ClearAccum(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void GLAPIENTRY xgl_real_Accum(GLenum op, GLfloat value);
void GLAPIENTRY xgl_real_ClearStencil(GLint s);
void GLAPIENTRY xgl_real_StencilMask(GLuint mask);
void GLAPIENTRY xgl_real_StencilFunc(GLenum func, GLint ref, GLuint mask);
void GLAPIENTRY xgl_real_StencilOp(GLenum sfail, GLenum zfail, GLenum zpass);

/* Raster / pixel / bitmap ops */
void GLAPIENTRY xgl_real_RasterPos2f(GLfloat x, GLfloat y);
void GLAPIENTRY xgl_real_RasterPos2i(GLint x, GLint y);
void GLAPIENTRY xgl_real_RasterPos2d(GLdouble x, GLdouble y);
void GLAPIENTRY xgl_real_RasterPos2s(GLshort x, GLshort y);
void GLAPIENTRY xgl_real_RasterPos3f(GLfloat x, GLfloat y, GLfloat z);
void GLAPIENTRY xgl_real_RasterPos3i(GLint x, GLint y, GLint z);
void GLAPIENTRY xgl_real_RasterPos3d(GLdouble x, GLdouble y, GLdouble z);
void GLAPIENTRY xgl_real_RasterPos3s(GLshort x, GLshort y, GLshort z);
void GLAPIENTRY xgl_real_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void GLAPIENTRY xgl_real_RasterPos4i(GLint x, GLint y, GLint z, GLint w);
void GLAPIENTRY xgl_real_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void GLAPIENTRY xgl_real_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w);
void GLAPIENTRY xgl_real_RasterPos2fv(const GLfloat *v);
void GLAPIENTRY xgl_real_RasterPos2iv(const GLint *v);
void GLAPIENTRY xgl_real_RasterPos2dv(const GLdouble *v);
void GLAPIENTRY xgl_real_RasterPos2sv(const GLshort *v);
void GLAPIENTRY xgl_real_RasterPos3fv(const GLfloat *v);
void GLAPIENTRY xgl_real_RasterPos3iv(const GLint *v);
void GLAPIENTRY xgl_real_RasterPos3dv(const GLdouble *v);
void GLAPIENTRY xgl_real_RasterPos3sv(const GLshort *v);
void GLAPIENTRY xgl_real_RasterPos4fv(const GLfloat *v);
void GLAPIENTRY xgl_real_RasterPos4iv(const GLint *v);
void GLAPIENTRY xgl_real_RasterPos4dv(const GLdouble *v);
void GLAPIENTRY xgl_real_RasterPos4sv(const GLshort *v);
void GLAPIENTRY xgl_real_DrawPixels(GLsizei w, GLsizei h, GLenum format, GLenum type, const GLvoid *pixels);
void GLAPIENTRY xgl_real_Bitmap(GLsizei w, GLsizei h, GLfloat xo, GLfloat yo, GLfloat xm, GLfloat ym, const GLubyte *bitmap);
void GLAPIENTRY xgl_real_PixelZoom(GLfloat xf, GLfloat yf);
void GLAPIENTRY xgl_real_PixelStorei(GLenum pname, GLint param);
void GLAPIENTRY xgl_real_PixelStoref(GLenum pname, GLfloat param);
void GLAPIENTRY xgl_real_PixelTransferi(GLenum pname, GLint param);
void GLAPIENTRY xgl_real_PixelTransferf(GLenum pname, GLfloat param);
void GLAPIENTRY xgl_real_PixelMapfv(GLenum map, GLint n, const GLfloat *v);
void GLAPIENTRY xgl_real_PixelMapuiv(GLenum map, GLint n, const GLuint *v);
void GLAPIENTRY xgl_real_PixelMapusv(GLenum map, GLint n, const GLushort *v);
void GLAPIENTRY xgl_real_ReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, GLvoid *pixels);
void GLAPIENTRY xgl_real_CopyPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum type);
void GLAPIENTRY xgl_real_CopyTexImage1D(GLenum target, GLint level, GLenum internalFormat,
                                        GLint x, GLint y, GLsizei w, GLint border);
void GLAPIENTRY xgl_real_CopyTexSubImage1D(GLenum target, GLint level, GLint xoff, GLint x, GLint y, GLsizei w);
void GLAPIENTRY xgl_real_CopyTexImage2D(GLenum target, GLint level, GLenum internalFormat,
                                        GLint x, GLint y, GLsizei w, GLsizei h, GLint border);
void GLAPIENTRY xgl_real_CopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff,
                                           GLint x, GLint y, GLsizei w, GLsizei h);

/* Vertex-array completeness */
void GLAPIENTRY xgl_real_ArrayElement(GLint i);
void GLAPIENTRY xgl_real_InterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer);
void GLAPIENTRY xgl_real_IndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void GLAPIENTRY xgl_real_EdgeFlagPointer(GLsizei stride, const GLvoid *pointer);
void GLAPIENTRY xgl_real_PushClientAttrib(GLbitfield mask);
void GLAPIENTRY xgl_real_PopClientAttrib(void);
void GLAPIENTRY xgl_real_GetPointerv(GLenum pname, GLvoid **params);

/* Texture object queries */
GLboolean GLAPIENTRY xgl_real_IsTexture(GLuint texture);
GLboolean GLAPIENTRY xgl_real_AreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences);
void GLAPIENTRY xgl_real_PrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities);

/* Complete glGet* family */
void GLAPIENTRY xgl_real_GetBooleanv(GLenum pname, GLboolean *params);
void GLAPIENTRY xgl_real_GetDoublev(GLenum pname, GLdouble *params);
void GLAPIENTRY xgl_real_GetLightfv(GLenum light, GLenum pname, GLfloat *params);
void GLAPIENTRY xgl_real_GetLightiv(GLenum light, GLenum pname, GLint *params);
void GLAPIENTRY xgl_real_GetMaterialfv(GLenum face, GLenum pname, GLfloat *params);
void GLAPIENTRY xgl_real_GetMaterialiv(GLenum face, GLenum pname, GLint *params);
void GLAPIENTRY xgl_real_GetTexEnvfv(GLenum t, GLenum p, GLfloat *params);
void GLAPIENTRY xgl_real_GetTexEnviv(GLenum t, GLenum p, GLint *params);
void GLAPIENTRY xgl_real_GetTexParameterfv(GLenum t, GLenum p, GLfloat *params);
void GLAPIENTRY xgl_real_GetTexParameteriv(GLenum t, GLenum p, GLint *params);
void GLAPIENTRY xgl_real_GetTexLevelParameterfv(GLenum t, GLint level, GLenum p, GLfloat *params);
void GLAPIENTRY xgl_real_GetTexLevelParameteriv(GLenum t, GLint level, GLenum p, GLint *params);
void GLAPIENTRY xgl_real_GetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
void GLAPIENTRY xgl_real_GetMapfv(GLenum target, GLenum query, GLfloat *v);
void GLAPIENTRY xgl_real_GetMapdv(GLenum target, GLenum query, GLdouble *v);
void GLAPIENTRY xgl_real_GetMapiv(GLenum target, GLenum query, GLint *v);
void GLAPIENTRY xgl_real_GetPixelMapfv(GLenum map, GLfloat *values);
void GLAPIENTRY xgl_real_GetPixelMapuiv(GLenum map, GLuint *values);
void GLAPIENTRY xgl_real_GetPixelMapusv(GLenum map, GLushort *values);

/* Selection + feedback */
void GLAPIENTRY xgl_real_SelectBuffer(GLsizei size, GLuint *buffer);
void GLAPIENTRY xgl_real_FeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer);
GLint GLAPIENTRY xgl_real_RenderMode(GLenum mode);
void GLAPIENTRY xgl_real_InitNames(void);
void GLAPIENTRY xgl_real_LoadName(GLuint name);
void GLAPIENTRY xgl_real_PushName(GLuint name);
void GLAPIENTRY xgl_real_PopName(void);
void GLAPIENTRY xgl_real_PassThrough(GLfloat token);

/* Evaluators */
void GLAPIENTRY xgl_real_Map1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
void GLAPIENTRY xgl_real_Map1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
void GLAPIENTRY xgl_real_Map2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
                               GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
void GLAPIENTRY xgl_real_Map2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
                               GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
void GLAPIENTRY xgl_real_MapGrid1f(GLint un, GLfloat u1, GLfloat u2);
void GLAPIENTRY xgl_real_MapGrid1d(GLint un, GLdouble u1, GLdouble u2);
void GLAPIENTRY xgl_real_MapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
void GLAPIENTRY xgl_real_MapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
void GLAPIENTRY xgl_real_EvalCoord1f(GLfloat u);
void GLAPIENTRY xgl_real_EvalCoord1d(GLdouble u);
void GLAPIENTRY xgl_real_EvalCoord1fv(const GLfloat *u);
void GLAPIENTRY xgl_real_EvalCoord1dv(const GLdouble *u);
void GLAPIENTRY xgl_real_EvalCoord2f(GLfloat u, GLfloat v);
void GLAPIENTRY xgl_real_EvalCoord2d(GLdouble u, GLdouble v);
void GLAPIENTRY xgl_real_EvalCoord2fv(const GLfloat *u);
void GLAPIENTRY xgl_real_EvalCoord2dv(const GLdouble *u);
void GLAPIENTRY xgl_real_EvalMesh1(GLenum mode, GLint i1, GLint i2);
void GLAPIENTRY xgl_real_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
void GLAPIENTRY xgl_real_EvalPoint1(GLint i);
void GLAPIENTRY xgl_real_EvalPoint2(GLint i, GLint j);

/* Free all display-list storage held by a context (icd.c DrvDeleteContext). */
void XglFreeLists(PXGL_CONTEXT c);
/* Free evaluator map storage held by a context (icd.c DrvDeleteContext). */
void XglFreeEvalMaps(PXGL_CONTEXT c);
