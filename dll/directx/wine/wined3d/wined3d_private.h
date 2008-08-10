/*
 * Direct3D wine internal private include file
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Jason Edmeades
 * Copyright 2005 Oliver Stieber
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINED3D_PRIVATE_H
#define __WINE_WINED3D_PRIVATE_H

#include <stdarg.h>
#include <math.h>
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "wined3d_private_types.h"
#include "wine/wined3d_interface.h"
#include "wine/wined3d_caps.h"
#include "wine/wined3d_gl.h"
#include "wine/list.h"

#define  ceilf(x) (float)ceil((double)x)
/* Hash table functions */
typedef unsigned int (hash_function_t)(void *key);
typedef BOOL (compare_function_t)(void *keya, void *keyb);

typedef struct {
    void *key;
    void *value;
    unsigned int hash;
    struct list entry;
} hash_table_entry_t;

typedef struct {
    hash_function_t *hash_function;
    compare_function_t *compare_function;
    struct list *buckets;
    unsigned int bucket_count;
    hash_table_entry_t *entries;
    unsigned int entry_count;
    struct list free_entries;
    unsigned int count;
    unsigned int grow_size;
    unsigned int shrink_size;
} hash_table_t;

hash_table_t *hash_table_create(hash_function_t *hash_function, compare_function_t *compare_function);
void hash_table_destroy(hash_table_t *table);
void *hash_table_get(hash_table_t *table, void *key);
void hash_table_put(hash_table_t *table, void *key, void *value);
void hash_table_remove(hash_table_t *table, void *key);

/* Device caps */
#define MAX_PALETTES            256
#define MAX_STREAMS             16
#define MAX_TEXTURES            8
#define MAX_FRAGMENT_SAMPLERS   16
#define MAX_VERTEX_SAMPLERS     4
#define MAX_COMBINED_SAMPLERS   (MAX_FRAGMENT_SAMPLERS + MAX_VERTEX_SAMPLERS)
#define MAX_ACTIVE_LIGHTS       8
#define MAX_CLIPPLANES          WINED3DMAXUSERCLIPPLANES
#define MAX_LEVELS              256

#define MAX_CONST_I 16
#define MAX_CONST_B 16

/* Used for CreateStateBlock */
#define NUM_SAVEDPIXELSTATES_R     35
#define NUM_SAVEDPIXELSTATES_T     18
#define NUM_SAVEDPIXELSTATES_S     12
#define NUM_SAVEDVERTEXSTATES_R    34
#define NUM_SAVEDVERTEXSTATES_T    2
#define NUM_SAVEDVERTEXSTATES_S    1

extern const DWORD SavedPixelStates_R[NUM_SAVEDPIXELSTATES_R];
extern const DWORD SavedPixelStates_T[NUM_SAVEDPIXELSTATES_T];
extern const DWORD SavedPixelStates_S[NUM_SAVEDPIXELSTATES_S];
extern const DWORD SavedVertexStates_R[NUM_SAVEDVERTEXSTATES_R];
extern const DWORD SavedVertexStates_T[NUM_SAVEDVERTEXSTATES_T];
extern const DWORD SavedVertexStates_S[NUM_SAVEDVERTEXSTATES_S];

typedef enum _WINELOOKUP {
    WINELOOKUP_WARPPARAM = 0,
    WINELOOKUP_MAGFILTER = 1,
    MAX_LOOKUPS          = 2
} WINELOOKUP;

extern int minLookup[MAX_LOOKUPS];
extern int maxLookup[MAX_LOOKUPS];
extern DWORD *stateLookup[MAX_LOOKUPS];

extern DWORD minMipLookup[WINED3DTEXF_ANISOTROPIC + 1][WINED3DTEXF_LINEAR + 1];

typedef struct _WINED3DGLTYPE {
    int         d3dType;
    GLint       size;
    GLenum      glType;
    GLboolean   normalized;
    int         typesize;
} WINED3DGLTYPE;

/* NOTE: Make sure these are in the correct numerical order. (see /include/wined3d_types.h) */
static WINED3DGLTYPE const glTypeLookup[WINED3DDECLTYPE_UNUSED] = {
                                  {WINED3DDECLTYPE_FLOAT1,    1, GL_FLOAT           , GL_FALSE ,sizeof(float)},
                                  {WINED3DDECLTYPE_FLOAT2,    2, GL_FLOAT           , GL_FALSE ,sizeof(float)},
                                  {WINED3DDECLTYPE_FLOAT3,    3, GL_FLOAT           , GL_FALSE ,sizeof(float)},
                                  {WINED3DDECLTYPE_FLOAT4,    4, GL_FLOAT           , GL_FALSE ,sizeof(float)},
                                  {WINED3DDECLTYPE_D3DCOLOR,  4, GL_UNSIGNED_BYTE   , GL_TRUE  ,sizeof(BYTE)},
                                  {WINED3DDECLTYPE_UBYTE4,    4, GL_UNSIGNED_BYTE   , GL_FALSE ,sizeof(BYTE)},
                                  {WINED3DDECLTYPE_SHORT2,    2, GL_SHORT           , GL_FALSE ,sizeof(short int)},
                                  {WINED3DDECLTYPE_SHORT4,    4, GL_SHORT           , GL_FALSE ,sizeof(short int)},
                                  {WINED3DDECLTYPE_UBYTE4N,   4, GL_UNSIGNED_BYTE   , GL_TRUE  ,sizeof(BYTE)},
                                  {WINED3DDECLTYPE_SHORT2N,   2, GL_SHORT           , GL_TRUE  ,sizeof(short int)},
                                  {WINED3DDECLTYPE_SHORT4N,   4, GL_SHORT           , GL_TRUE  ,sizeof(short int)},
                                  {WINED3DDECLTYPE_USHORT2N,  2, GL_UNSIGNED_SHORT  , GL_TRUE  ,sizeof(short int)},
                                  {WINED3DDECLTYPE_USHORT4N,  4, GL_UNSIGNED_SHORT  , GL_TRUE  ,sizeof(short int)},
                                  {WINED3DDECLTYPE_UDEC3,     3, GL_UNSIGNED_SHORT  , GL_FALSE ,sizeof(short int)},
                                  {WINED3DDECLTYPE_DEC3N,     3, GL_SHORT           , GL_TRUE  ,sizeof(short int)},
                                  /* We should do an extension check for NV_HALF_FLOAT. However, without NV_HALF_FLOAT
                                   * we won't be able to load the data at all, so at least for the moment it wouldn't
                                   * gain us much. */
                                  {WINED3DDECLTYPE_FLOAT16_2, 2, GL_HALF_FLOAT_NV   , GL_FALSE ,sizeof(GLhalfNV)},
                                  {WINED3DDECLTYPE_FLOAT16_4, 4, GL_HALF_FLOAT_NV   , GL_FALSE ,sizeof(GLhalfNV)}};

#define WINED3D_ATR_TYPE(type)          glTypeLookup[type].d3dType
#define WINED3D_ATR_SIZE(type)          glTypeLookup[type].size
#define WINED3D_ATR_GLTYPE(type)        glTypeLookup[type].glType
#define WINED3D_ATR_NORMALIZED(type)    glTypeLookup[type].normalized
#define WINED3D_ATR_TYPESIZE(type)      glTypeLookup[type].typesize

/**
 * Settings 
 */
#define VS_NONE    0
#define VS_HW      1

#define PS_NONE    0
#define PS_HW      1

#define VBO_NONE   0
#define VBO_HW     1

#define NP2_NONE   0
#define NP2_REPACK 1
#define NP2_NATIVE 2

#define ORM_BACKBUFFER  0
#define ORM_PBUFFER     1
#define ORM_FBO         2

#define SHADER_ARB  1
#define SHADER_GLSL 2
#define SHADER_NONE 3

#define RTL_DISABLE   -1
#define RTL_AUTO       0
#define RTL_READDRAW   1
#define RTL_READTEX    2
#define RTL_TEXDRAW    3
#define RTL_TEXTEX     4

/* NOTE: When adding fields to this structure, make sure to update the default
 * values in wined3d_main.c as well. */
typedef struct wined3d_settings_s {
/* vertex and pixel shader modes */
  int vs_mode;
  int ps_mode;
  int vbo_mode;
/* Ideally, we don't want the user to have to request GLSL.  If the hardware supports GLSL,
    we should use it.  However, until it's fully implemented, we'll leave it as a registry
    setting for developers. */
  BOOL glslRequested;
  int offscreen_rendering_mode;
  int rendertargetlock_mode;
/* Memory tracking and object counting */
  unsigned int emulated_textureram;
  char *logo;
} wined3d_settings_t;

extern wined3d_settings_t wined3d_settings;

/* Shader backends */
struct SHADER_OPCODE_ARG;

typedef struct {
    void (*shader_select)(IWineD3DDevice *iface, BOOL usePS, BOOL useVS);
    void (*shader_select_depth_blt)(IWineD3DDevice *iface);
    void (*shader_load_constants)(IWineD3DDevice *iface, char usePS, char useVS);
    void (*shader_cleanup)(IWineD3DDevice *iface);
    void (*shader_color_correction)(struct SHADER_OPCODE_ARG *arg);
    void (*shader_destroy)(IWineD3DBaseShader *iface);
} shader_backend_t;

extern const shader_backend_t glsl_shader_backend;
extern const shader_backend_t arb_program_shader_backend;
extern const shader_backend_t none_shader_backend;

/* X11 locking */

extern void (*wine_tsx11_lock_ptr)(void);
extern void (*wine_tsx11_unlock_ptr)(void);

/* As GLX relies on X, this is needed */
extern int num_lock;

#if 0
#define ENTER_GL() ++num_lock; if (num_lock > 1) FIXME("Recursive use of GL lock to: %d\n", num_lock); wine_tsx11_lock_ptr()
#define LEAVE_GL() if (num_lock != 1) FIXME("Recursive use of GL lock: %d\n", num_lock); --num_lock; wine_tsx11_unlock_ptr()
#else
#define ENTER_GL() wine_tsx11_lock_ptr()
#define LEAVE_GL() wine_tsx11_unlock_ptr()
#endif

/*****************************************************************************
 * Defines
 */

/* GL related defines */
/* ------------------ */
#define GL_SUPPORT(ExtName)           (GLINFO_LOCATION.supported[ExtName] != 0)
#define GL_LIMITS(ExtName)            (GLINFO_LOCATION.max_##ExtName)
#define GL_EXTCALL(FuncName)          (GLINFO_LOCATION.FuncName)
#define GL_VEND(_VendName)            (GLINFO_LOCATION.gl_vendor == VENDOR_##_VendName ? TRUE : FALSE)

#define D3DCOLOR_B_R(dw) (((dw) >> 16) & 0xFF)
#define D3DCOLOR_B_G(dw) (((dw) >>  8) & 0xFF)
#define D3DCOLOR_B_B(dw) (((dw) >>  0) & 0xFF)
#define D3DCOLOR_B_A(dw) (((dw) >> 24) & 0xFF)

#define D3DCOLOR_R(dw) (((float) (((dw) >> 16) & 0xFF)) / 255.0f)
#define D3DCOLOR_G(dw) (((float) (((dw) >>  8) & 0xFF)) / 255.0f)
#define D3DCOLOR_B(dw) (((float) (((dw) >>  0) & 0xFF)) / 255.0f)
#define D3DCOLOR_A(dw) (((float) (((dw) >> 24) & 0xFF)) / 255.0f)

#define D3DCOLORTOGLFLOAT4(dw, vec) \
  (vec)[0] = D3DCOLOR_R(dw); \
  (vec)[1] = D3DCOLOR_G(dw); \
  (vec)[2] = D3DCOLOR_B(dw); \
  (vec)[3] = D3DCOLOR_A(dw);

/* DirectX Device Limits */
/* --------------------- */
#define MAX_LEVELS  256  /* Maximum number of mipmap levels. Guessed at 256 */

#define MAX_STREAMS  16  /* Maximum possible streams - used for fixed size arrays
                            See MaxStreams in MSDN under GetDeviceCaps */
                         /* Maximum number of constants provided to the shaders */
#define HIGHEST_TRANSFORMSTATE 512 
                         /* Highest value in WINED3DTRANSFORMSTATETYPE */
#define MAX_PALETTES      256

/* Checking of API calls */
/* --------------------- */
#define checkGLcall(A)                                          \
{                                                               \
    GLint err = glGetError();                                   \
    if (err == GL_NO_ERROR) {                                   \
       TRACE("%s call ok %s / %d\n", A, __FILE__, __LINE__);    \
                                                                \
    } else do {                                                 \
        FIXME(">>>>>>>>>>>>>>>>> %s (%#x) from %s @ %s / %d\n", \
            debug_glerror(err), err, A, __FILE__, __LINE__);    \
       err = glGetError();                                      \
    } while (err != GL_NO_ERROR);                               \
} 

/* Trace routines / diagnostics */
/* ---------------------------- */

/* Dump out a matrix and copy it */
#define conv_mat(mat,gl_mat)                                                                \
do {                                                                                        \
    TRACE("%f %f %f %f\n", (mat)->u.s._11, (mat)->u.s._12, (mat)->u.s._13, (mat)->u.s._14); \
    TRACE("%f %f %f %f\n", (mat)->u.s._21, (mat)->u.s._22, (mat)->u.s._23, (mat)->u.s._24); \
    TRACE("%f %f %f %f\n", (mat)->u.s._31, (mat)->u.s._32, (mat)->u.s._33, (mat)->u.s._34); \
    TRACE("%f %f %f %f\n", (mat)->u.s._41, (mat)->u.s._42, (mat)->u.s._43, (mat)->u.s._44); \
    memcpy(gl_mat, (mat), 16 * sizeof(float));                                              \
} while (0)

/* Macro to dump out the current state of the light chain */
#define DUMP_LIGHT_CHAIN()                    \
{                                             \
  PLIGHTINFOEL *el = This->stateBlock->lights;\
  while (el) {                                \
    TRACE("Light %p (glIndex %ld, d3dIndex %ld, enabled %d)\n", el, el->glIndex, el->OriginalIndex, el->lightEnabled);\
    el = el->next;                            \
  }                                           \
}

/* Trace vector and strided data information */
#define TRACE_VECTOR(name) TRACE( #name "=(%f, %f, %f, %f)\n", name.x, name.y, name.z, name.w);
#define TRACE_STRIDED(sd,name) TRACE( #name "=(data:%p, stride:%d, type:%d, vbo %d, stream %u)\n", \
        sd->u.s.name.lpData, sd->u.s.name.dwStride, sd->u.s.name.dwType, sd->u.s.name.VBO, sd->u.s.name.streamNo);

/* Defines used for optimizations */

/*    Only reapply what is necessary */
#define REAPPLY_ALPHAOP  0x0001
#define REAPPLY_ALL      0xFFFF

/* Advance declaration of structures to satisfy compiler */
typedef struct IWineD3DStateBlockImpl IWineD3DStateBlockImpl;
typedef struct IWineD3DSurfaceImpl    IWineD3DSurfaceImpl;
typedef struct IWineD3DPaletteImpl    IWineD3DPaletteImpl;
typedef struct IWineD3DDeviceImpl     IWineD3DDeviceImpl;

/* Global variables */
extern const float identity[16];

/*****************************************************************************
 * Compilable extra diagnostics
 */

/* Trace information per-vertex: (extremely high amount of trace) */
#if 0 /* NOTE: Must be 0 in cvs */
# define VTRACE(A) TRACE A
#else 
# define VTRACE(A) 
#endif

/* Checking of per-vertex related GL calls */
/* --------------------- */
#define vcheckGLcall(A)                                         \
{                                                               \
    GLint err = glGetError();                                   \
    if (err == GL_NO_ERROR) {                                   \
       VTRACE(("%s call ok %s / %d\n", A, __FILE__, __LINE__)); \
                                                                \
    } else do {                                                 \
        FIXME(">>>>>>>>>>>>>>>>> %s (%#x) from %s @ %s / %d\n", \
            debug_glerror(err), err, A, __FILE__, __LINE__);    \
       err = glGetError();                                      \
    } while (err != GL_NO_ERROR);                               \
}

/* TODO: Confirm each of these works when wined3d move completed */
#if 0 /* NOTE: Must be 0 in cvs */
  /* To avoid having to get gigabytes of trace, the following can be compiled in, and at the start
     of each frame, a check is made for the existence of C:\D3DTRACE, and if it exists d3d trace
     is enabled, and if it doesn't exist it is disabled. */
# define FRAME_DEBUGGING
  /*  Adding in the SINGLE_FRAME_DEBUGGING gives a trace of just what makes up a single frame, before
      the file is deleted                                                                            */
# if 1 /* NOTE: Must be 1 in cvs, as this is mostly more useful than a trace from program start */
#  define SINGLE_FRAME_DEBUGGING
# endif  
  /* The following, when enabled, lets you see the makeup of the frame, by drawprimitive calls.
     It can only be enabled when FRAME_DEBUGGING is also enabled                               
     The contents of the back buffer are written into /tmp/backbuffer_* after each primitive 
     array is drawn.                                                                            */
# if 0 /* NOTE: Must be 0 in cvs, as this give a lot of ppm files when compiled in */                                                                                       
#  define SHOW_FRAME_MAKEUP 1
# endif  
  /* The following, when enabled, lets you see the makeup of the all the textures used during each
     of the drawprimitive calls. It can only be enabled when SHOW_FRAME_MAKEUP is also enabled.
     The contents of the textures assigned to each stage are written into 
     /tmp/texture_*_<Stage>.ppm after each primitive array is drawn.                            */
# if 0 /* NOTE: Must be 0 in cvs, as this give a lot of ppm files when compiled in */
#  define SHOW_TEXTURE_MAKEUP 0
# endif  
extern BOOL isOn;
extern BOOL isDumpingFrames;
extern LONG primCounter;
#endif

/*****************************************************************************
 * Prototypes
 */

/* Routine common to the draw primitive and draw indexed primitive routines */
void drawPrimitive(IWineD3DDevice *iface,
                    int PrimitiveType,
                    long NumPrimitives,
                    /* for Indexed: */
                    long  StartVertexIndex,
                    UINT  numberOfVertices,
                    long  StartIdx,
                    short idxBytes,
                    const void *idxData,
                    int   minIndex);

void primitiveDeclarationConvertToStridedData(
     IWineD3DDevice *iface,
     BOOL useVertexShaderFunction,
     WineDirect3DVertexStridedData *strided,
     BOOL *fixup);

DWORD get_flexible_vertex_size(DWORD d3dvtVertexType);

#define eps 1e-8

#define GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, tex_num) \
    (((((d3dvtVertexType) >> (16 + (2 * (tex_num)))) + 1) & 0x03) + 1)

/* Routines and structures related to state management */
typedef struct WineD3DContext WineD3DContext;
typedef void (*APPLYSTATEFUNC)(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *ctx);

#define STATE_RENDER(a) (a)
#define STATE_IS_RENDER(a) ((a) >= STATE_RENDER(1) && (a) <= STATE_RENDER(WINEHIGHEST_RENDER_STATE))

#define STATE_TEXTURESTAGE(stage, num) (STATE_RENDER(WINEHIGHEST_RENDER_STATE) + (stage) * WINED3D_HIGHEST_TEXTURE_STATE + (num))
#define STATE_IS_TEXTURESTAGE(a) ((a) >= STATE_TEXTURESTAGE(0, 1) && (a) <= STATE_TEXTURESTAGE(MAX_TEXTURES - 1, WINED3D_HIGHEST_TEXTURE_STATE))

/* + 1 because samplers start with 0 */
#define STATE_SAMPLER(num) (STATE_TEXTURESTAGE(MAX_TEXTURES - 1, WINED3D_HIGHEST_TEXTURE_STATE) + 1 + (num))
#define STATE_IS_SAMPLER(num) ((num) >= STATE_SAMPLER(0) && (num) <= STATE_SAMPLER(MAX_COMBINED_SAMPLERS - 1))

#define STATE_PIXELSHADER (STATE_SAMPLER(MAX_COMBINED_SAMPLERS - 1) + 1)
#define STATE_IS_PIXELSHADER(a) ((a) == STATE_PIXELSHADER)

#define STATE_TRANSFORM(a) (STATE_PIXELSHADER + (a))
#define STATE_IS_TRANSFORM(a) ((a) >= STATE_TRANSFORM(1) && (a) <= STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(255)))

#define STATE_STREAMSRC (STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(255)) + 1)
#define STATE_IS_STREAMSRC(a) ((a) == STATE_STREAMSRC)
#define STATE_INDEXBUFFER (STATE_STREAMSRC + 1)
#define STATE_IS_INDEXBUFFER(a) ((a) == STATE_INDEXBUFFER)

#define STATE_VDECL (STATE_INDEXBUFFER + 1)
#define STATE_IS_VDECL(a) ((a) == STATE_VDECL)

#define STATE_VSHADER (STATE_VDECL + 1)
#define STATE_IS_VSHADER(a) ((a) == STATE_VSHADER)

#define STATE_VIEWPORT (STATE_VSHADER + 1)
#define STATE_IS_VIEWPORT(a) ((a) == STATE_VIEWPORT)

#define STATE_VERTEXSHADERCONSTANT (STATE_VIEWPORT + 1)
#define STATE_PIXELSHADERCONSTANT (STATE_VERTEXSHADERCONSTANT + 1)
#define STATE_IS_VERTEXSHADERCONSTANT(a) ((a) == STATE_VERTEXSHADERCONSTANT)
#define STATE_IS_PIXELSHADERCONSTANT(a) ((a) == STATE_PIXELSHADERCONSTANT)

#define STATE_ACTIVELIGHT(a) (STATE_PIXELSHADERCONSTANT + (a) + 1)
#define STATE_IS_ACTIVELIGHT(a) ((a) >= STATE_ACTIVELIGHT(0) && (a) < STATE_ACTIVELIGHT(MAX_ACTIVE_LIGHTS))

#define STATE_SCISSORRECT (STATE_ACTIVELIGHT(MAX_ACTIVE_LIGHTS - 1) + 1)
#define STATE_IS_SCISSORRECT(a) ((a) == STATE_SCISSORRECT)

#define STATE_CLIPPLANE(a) (STATE_SCISSORRECT + 1 + (a))
#define STATE_IS_CLIPPLANE(a) ((a) >= STATE_CLIPPLANE(0) && (a) <= STATE_CLIPPLANE(MAX_CLIPPLANES - 1))

#define STATE_MATERIAL (STATE_CLIPPLANE(MAX_CLIPPLANES))

#define STATE_FRONTFACE (STATE_MATERIAL + 1)

#define STATE_HIGHEST (STATE_FRONTFACE)

struct StateEntry
{
    DWORD           representative;
    APPLYSTATEFUNC  apply;
};

/* Global state table */
extern const struct StateEntry StateTable[];

/* The new context manager that should deal with onscreen and offscreen rendering */
struct WineD3DContext {
    /* State dirtification
     * dirtyArray is an array that contains markers for dirty states. numDirtyEntries states are dirty, their numbers are in indices
     * 0...numDirtyEntries - 1. isStateDirty is a redundant copy of the dirtyArray. Technically only one of them would be needed,
     * but with the help of both it is easy to find out if a state is dirty(just check the array index), and for applying dirty states
     * only numDirtyEntries array elements have to be checked, not STATE_HIGHEST states.
     */
    DWORD                   dirtyArray[STATE_HIGHEST + 1]; /* Won't get bigger than that, a state is never marked dirty 2 times */
    DWORD                   numDirtyEntries;
    DWORD                   isStateDirty[STATE_HIGHEST/32 + 1]; /* Bitmap to find out quickly if a state is dirty */

    IWineD3DSurface         *surface;
    DWORD                   tid;    /* Thread ID which owns this context at the moment */

    /* Stores some inforation about the context state for optimization */
    GLint                   last_draw_buffer;
    BOOL                    last_was_rhw;      /* true iff last draw_primitive was in xyzrhw mode */
    BOOL                    last_was_pshader;
    BOOL                    last_was_vshader;
    BOOL                    last_was_foggy_shader;
    BOOL                    namedArraysLoaded, numberedArraysLoaded;
    BOOL                    lastWasPow2Texture[MAX_TEXTURES];
    GLenum                  tracking_parm;     /* Which source is tracking current colour         */
    unsigned char           num_untracked_materials;
    GLenum                  untracked_materials[2];
    BOOL                    last_was_blit, last_was_ckey;
    char                    texShaderBumpMap;
    BOOL                    fog_coord;

    /* The actual opengl context */
    HGLRC                   glCtx;
    HWND                    win_handle;
    HDC                     hdc;
    HPBUFFERARB             pbuffer;
    BOOL                    isPBuffer;
};

typedef enum ContextUsage {
    CTXUSAGE_RESOURCELOAD       = 1,    /* Only loads textures: No State is applied */
    CTXUSAGE_DRAWPRIM           = 2,    /* OpenGL states are set up for blitting DirectDraw surfacs */
    CTXUSAGE_BLIT               = 3,    /* OpenGL states are set up 3D drawing */
    CTXUSAGE_CLEAR              = 4,    /* Drawable and states are set up for clearing */
} ContextUsage;

void ActivateContext(IWineD3DDeviceImpl *device, IWineD3DSurface *target, ContextUsage usage);
WineD3DContext *CreateContext(IWineD3DDeviceImpl *This, IWineD3DSurfaceImpl *target, HWND win, BOOL create_pbuffer, const WINED3DPRESENT_PARAMETERS *pPresentParms);
void DestroyContext(IWineD3DDeviceImpl *This, WineD3DContext *context);
void apply_fbo_state(IWineD3DDevice *iface);

/* Macros for doing basic GPU detection based on opengl capabilities */
#define WINE_D3D6_CAPABLE(gl_info) (gl_info->supported[ARB_MULTITEXTURE])
#define WINE_D3D7_CAPABLE(gl_info) (gl_info->supported[ARB_TEXTURE_COMPRESSION] && gl_info->supported[ARB_TEXTURE_CUBE_MAP] && gl_info->supported[ARB_TEXTURE_ENV_DOT3])
#define WINE_D3D8_CAPABLE(gl_info) WINE_D3D7_CAPABLE(gl_info) && (gl_info->supported[ARB_MULTISAMPLE] && gl_info->supported[ARB_TEXTURE_BORDER_CLAMP])
#define WINE_D3D9_CAPABLE(gl_info) WINE_D3D8_CAPABLE(gl_info) && (gl_info->supported[ARB_FRAGMENT_PROGRAM] && gl_info->supported[ARB_VERTEX_SHADER])

/* Default callbacks for implicit object destruction */
extern ULONG WINAPI D3DCB_DefaultDestroySurface(IWineD3DSurface *pSurface);

extern ULONG WINAPI D3DCB_DefaultDestroyVolume(IWineD3DVolume *pSurface);

/*****************************************************************************
 * Internal representation of a light
 */
typedef struct PLIGHTINFOEL PLIGHTINFOEL;
struct PLIGHTINFOEL {
    WINED3DLIGHT OriginalParms; /* Note D3D8LIGHT == D3D9LIGHT */
    DWORD        OriginalIndex;
    LONG         glIndex;
    BOOL         changed;
    BOOL         enabledChanged;

    /* Converted parms to speed up swapping lights */
    float                         lightPosn[4];
    float                         lightDirn[4];
    float                         exponent;
    float                         cutoff;

    struct list entry;
};

/* The default light parameters */
extern const WINED3DLIGHT WINED3D_default_light;

typedef struct WineD3D_PixelFormat
{
    int iPixelFormat; /* WGL pixel format */
    int redSize, greenSize, blueSize, alphaSize;
    int depthSize, stencilSize;
} WineD3D_PixelFormat;

/* The adapter structure */
typedef struct GLPixelFormatDesc GLPixelFormatDesc;
struct WineD3DAdapter
{
    POINT                   monitorPoint;
    WineD3D_GL_Info         gl_info;
    const char              *driver;
    const char              *description;
    WCHAR                   DeviceName[CCHDEVICENAME]; /* DeviceName for use with e.g. ChangeDisplaySettings */
    int                     nCfgs;
    WineD3D_PixelFormat     *cfgs;
    unsigned int            TextureRam; /* Amount of texture memory both video ram + AGP/TurboCache/HyperMemory/.. */
    unsigned int            UsedTextureRam;
};

extern BOOL InitAdapters(void);
extern BOOL initPixelFormats(WineD3D_GL_Info *gl_info);
extern long WineD3DAdapterChangeGLRam(IWineD3DDeviceImpl *D3DDevice, long glram);

/*****************************************************************************
 * High order patch management
 */
struct WineD3DRectPatch
{
    UINT                            Handle;
    float                          *mem;
    WineDirect3DVertexStridedData   strided;
    WINED3DRECTPATCH_INFO           RectPatchInfo;
    float                           numSegs[4];
    char                            has_normals, has_texcoords;
    struct list                     entry;
};

HRESULT tesselate_rectpatch(IWineD3DDeviceImpl *This, struct WineD3DRectPatch *patch);

/*****************************************************************************
 * IWineD3D implementation structure
 */
typedef struct IWineD3DImpl
{
    /* IUnknown fields */
    const IWineD3DVtbl     *lpVtbl;
    LONG                    ref;     /* Note: Ref counting not required */

    /* WineD3D Information */
    IUnknown               *parent;
    UINT                    dxVersion;
} IWineD3DImpl;

extern const IWineD3DVtbl IWineD3D_Vtbl;

/* TODO: setup some flags in the registry to enable, disable pbuffer support
(since it will break quite a few things until contexts are managed properly!) */
extern BOOL pbuffer_support;
/* allocate one pbuffer per surface */
extern BOOL pbuffer_per_surface;

/* A helper function that dumps a resource list */
void dumpResources(struct list *list);

/*****************************************************************************
 * IWineD3DDevice implementation structure
 */
struct IWineD3DDeviceImpl
{
    /* IUnknown fields      */
    const IWineD3DDeviceVtbl *lpVtbl;
    LONG                    ref;     /* Note: Ref counting not required */

    /* WineD3D Information  */
    IUnknown               *parent;
    IWineD3D               *wineD3D;
    struct WineD3DAdapter  *adapter;

    /* Window styles to restore when switching fullscreen mode */
    LONG                    style;
    LONG                    exStyle;

    /* X and GL Information */
    GLint                   maxConcurrentLights;
    GLenum                  offscreenBuffer;

    /* Selected capabilities */
    int vs_selected_mode;
    int ps_selected_mode;
    const shader_backend_t *shader_backend;
    hash_table_t *glsl_program_lookup;

    /* To store */
    BOOL                    view_ident;        /* true iff view matrix is identity                */
    BOOL                    untransformed;
    BOOL                    vertexBlendUsed;   /* To avoid needless setting of the blend matrices */
    unsigned char           surface_alignment; /* Line Alignment of surfaces                      */

    /* State block related */
    BOOL                    isRecordingState;
    IWineD3DStateBlockImpl *stateBlock;
    IWineD3DStateBlockImpl *updateStateBlock;
    BOOL                   isInDraw;

    /* Internal use fields  */
    WINED3DDEVICE_CREATION_PARAMETERS createParms;
    UINT                            adapterNo;
    WINED3DDEVTYPE                  devType;

    IWineD3DSwapChain     **swapchains;
    UINT                    NumberOfSwapChains;

    struct list             resources; /* a linked list to track resources created by the device */

    /* Render Target Support */
    IWineD3DSurface       **render_targets;
    IWineD3DSurface        *auto_depth_stencil_buffer;
    IWineD3DSurface       **fbo_color_attachments;
    IWineD3DSurface        *fbo_depth_attachment;

    IWineD3DSurface        *stencilBufferTarget;

    /* Caches to avoid unneeded context changes */
    IWineD3DSurface        *lastActiveRenderTarget;
    IWineD3DSwapChain      *lastActiveSwapChain;

    /* palettes texture management */
    PALETTEENTRY            palettes[MAX_PALETTES][256];
    UINT                    currentPalette;
    UINT                    paletteConversionShader;

    /* For rendering to a texture using glCopyTexImage */
    BOOL                    render_offscreen;
    WINED3D_DEPTHCOPYSTATE  depth_copy_state;
    GLuint                  fbo;
    GLuint                  src_fbo;
    GLuint                  dst_fbo;
    GLenum                  *draw_buffers;

    /* Cursor management */
    BOOL                    bCursorVisible;
    UINT                    xHotSpot;
    UINT                    yHotSpot;
    UINT                    xScreenSpace;
    UINT                    yScreenSpace;
    UINT                    cursorWidth, cursorHeight;
    GLuint                  cursorTexture;
    BOOL                    haveHardwareCursor;
    HCURSOR                 hardwareCursor;

    /* The Wine logo surface */
    IWineD3DSurface        *logo_surface;

    /* Textures for when no other textures are mapped */
    UINT                          dummyTextureName[MAX_TEXTURES];

    /* Debug stream management */
    BOOL                     debug;

    /* Device state management */
    HRESULT                 state;
    BOOL                    d3d_initialized;

    /* A flag to check for proper BeginScene / EndScene call pairs */
    BOOL inScene;

    /* process vertex shaders using software or hardware */
    BOOL softwareVertexProcessing;

    /* DirectDraw stuff */
    HWND ddraw_window;
    IWineD3DSurface *ddraw_primary;
    DWORD ddraw_width, ddraw_height;
    WINED3DFORMAT ddraw_format;
    BOOL ddraw_fullscreen;

    /* Final position fixup constant */
    float                       posFixup[4];

    /* With register combiners we can skip junk texture stages */
    DWORD                     texUnitMap[MAX_COMBINED_SAMPLERS];
    DWORD                     rev_tex_unit_map[MAX_COMBINED_SAMPLERS];
    BOOL                      fixed_function_usage_map[MAX_TEXTURES];

    /* Stream source management */
    WineDirect3DVertexStridedData strided_streams;
    WineDirect3DVertexStridedData *up_strided;
    BOOL                      useDrawStridedSlow;
    BOOL                      instancedDraw;

    /* Context management */
    WineD3DContext          **contexts;                  /* Dynamic array containing pointers to context structures */
    WineD3DContext          *activeContext;
    DWORD                   lastThread;
    UINT                    numContexts;
    WineD3DContext          *pbufferContext;             /* The context that has a pbuffer as drawable */
    DWORD                   pbufferWidth, pbufferHeight; /* Size of the buffer drawable */

    /* High level patch management */
#define PATCHMAP_SIZE 43
#define PATCHMAP_HASHFUNC(x) ((x) % PATCHMAP_SIZE) /* Primitive and simple function */
    struct list             patches[PATCHMAP_SIZE];
    struct WineD3DRectPatch *currentPatch;
};

extern const IWineD3DDeviceVtbl IWineD3DDevice_Vtbl;

void IWineD3DDeviceImpl_FindTexUnitMap(IWineD3DDeviceImpl *This);
void IWineD3DDeviceImpl_MarkStateDirty(IWineD3DDeviceImpl *This, DWORD state);
static inline BOOL isStateDirty(WineD3DContext *context, DWORD state) {
    DWORD idx = state >> 5;
    BYTE shift = state & 0x1f;
    return context->isStateDirty[idx] & (1 << shift);
}

/* Support for IWineD3DResource ::Set/Get/FreePrivateData. */
typedef struct PrivateData
{
    struct list entry;

    GUID tag;
    DWORD flags; /* DDSPD_* */
    DWORD uniqueness_value;

    union
    {
        LPVOID data;
        LPUNKNOWN object;
    } ptr;

    DWORD size;
} PrivateData;

/*****************************************************************************
 * IWineD3DResource implementation structure
 */
typedef struct IWineD3DResourceClass
{
    /* IUnknown fields */
    LONG                    ref;     /* Note: Ref counting not required */

    /* WineD3DResource Information */
    IUnknown               *parent;
    WINED3DRESOURCETYPE     resourceType;
    IWineD3DDeviceImpl     *wineD3DDevice;
    WINED3DPOOL             pool;
    UINT                    size;
    DWORD                   usage;
    WINED3DFORMAT           format;
    BYTE                   *allocatedMemory; /* Pointer to the real data location */
    BYTE                   *heapMemory; /* Pointer to the HeapAlloced block of memory */
    struct list             privateData;
    struct list             resource_list_entry;

} IWineD3DResourceClass;

typedef struct IWineD3DResourceImpl
{
    /* IUnknown & WineD3DResource Information     */
    const IWineD3DResourceVtbl *lpVtbl;
    IWineD3DResourceClass   resource;
} IWineD3DResourceImpl;

/* Tests show that the start address of resources is 32 byte aligned */
#define RESOURCE_ALIGNMENT 32

/*****************************************************************************
 * IWineD3DVertexBuffer implementation structure (extends IWineD3DResourceImpl)
 */
typedef struct IWineD3DVertexBufferImpl
{
    /* IUnknown & WineD3DResource Information     */
    const IWineD3DVertexBufferVtbl *lpVtbl;
    IWineD3DResourceClass     resource;

    /* WineD3DVertexBuffer specifics */
    DWORD                     fvf;

    /* Vertex buffer object support */
    GLuint                    vbo;
    BYTE                      Flags;
    LONG                      bindCount;

    UINT                      dirtystart, dirtyend;
    LONG                      lockcount;

    LONG                      declChanges, draws;
    /* Last description of the buffer */
    WineDirect3DVertexStridedData strided;
} IWineD3DVertexBufferImpl;

extern const IWineD3DVertexBufferVtbl IWineD3DVertexBuffer_Vtbl;

#define VBFLAG_LOAD           0x01    /* Data is written from allocatedMemory to the VBO */
#define VBFLAG_OPTIMIZED      0x02    /* Optimize has been called for the VB */
#define VBFLAG_DIRTY          0x04    /* Buffer data has been modified */
#define VBFLAG_HASDESC        0x08    /* A vertex description has been found */
#define VBFLAG_VBOCREATEFAIL  0x10    /* An attempt to create a vbo has failed */

/*****************************************************************************
 * IWineD3DIndexBuffer implementation structure (extends IWineD3DResourceImpl)
 */
typedef struct IWineD3DIndexBufferImpl
{
    /* IUnknown & WineD3DResource Information     */
    const IWineD3DIndexBufferVtbl *lpVtbl;
    IWineD3DResourceClass     resource;

    GLuint                    vbo;
    UINT                      dirtystart, dirtyend;
    LONG                      lockcount;

    /* WineD3DVertexBuffer specifics */
} IWineD3DIndexBufferImpl;

extern const IWineD3DIndexBufferVtbl IWineD3DIndexBuffer_Vtbl;

/*****************************************************************************
 * IWineD3DBaseTexture D3D- > openGL state map lookups
 */
#define WINED3DFUNC_NOTSUPPORTED  -2
#define WINED3DFUNC_UNIMPLEMENTED -1

typedef enum winetexturestates {
    WINED3DTEXSTA_ADDRESSU       = 0,
    WINED3DTEXSTA_ADDRESSV       = 1,
    WINED3DTEXSTA_ADDRESSW       = 2,
    WINED3DTEXSTA_BORDERCOLOR    = 3,
    WINED3DTEXSTA_MAGFILTER      = 4,
    WINED3DTEXSTA_MINFILTER      = 5,
    WINED3DTEXSTA_MIPFILTER      = 6,
    WINED3DTEXSTA_MAXMIPLEVEL    = 7,
    WINED3DTEXSTA_MAXANISOTROPY  = 8,
    WINED3DTEXSTA_SRGBTEXTURE    = 9,
    WINED3DTEXSTA_ELEMENTINDEX   = 10,
    WINED3DTEXSTA_DMAPOFFSET     = 11,
    WINED3DTEXSTA_TSSADDRESSW    = 12,
    MAX_WINETEXTURESTATES        = 13,
} winetexturestates;

/*****************************************************************************
 * IWineD3DBaseTexture implementation structure (extends IWineD3DResourceImpl)
 */
typedef struct IWineD3DBaseTextureClass
{
    UINT                    levels;
    BOOL                    dirty;
    UINT                    textureName;
    UINT                    LOD;
    WINED3DTEXTUREFILTERTYPE filterType;
    DWORD                   states[MAX_WINETEXTURESTATES];
    LONG                    bindCount;
    DWORD                   sampler;
    BOOL                    is_srgb;
    UINT                    srgb_mode_change_count;
    WINED3DFORMAT           shader_conversion_group;
    float                   pow2Matrix[16];
} IWineD3DBaseTextureClass;

typedef struct IWineD3DBaseTextureImpl
{
    /* IUnknown & WineD3DResource Information     */
    const IWineD3DBaseTextureVtbl *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

} IWineD3DBaseTextureImpl;

/*****************************************************************************
 * IWineD3DTexture implementation structure (extends IWineD3DBaseTextureImpl)
 */
typedef struct IWineD3DTextureImpl
{
    /* IUnknown & WineD3DResource/WineD3DBaseTexture Information     */
    const IWineD3DTextureVtbl *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

    /* IWineD3DTexture */
    IWineD3DSurface          *surfaces[MAX_LEVELS];
    
    UINT                      width;
    UINT                      height;

} IWineD3DTextureImpl;

extern const IWineD3DTextureVtbl IWineD3DTexture_Vtbl;

/*****************************************************************************
 * IWineD3DCubeTexture implementation structure (extends IWineD3DBaseTextureImpl)
 */
typedef struct IWineD3DCubeTextureImpl
{
    /* IUnknown & WineD3DResource/WineD3DBaseTexture Information     */
    const IWineD3DCubeTextureVtbl *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

    /* IWineD3DCubeTexture */
    IWineD3DSurface          *surfaces[6][MAX_LEVELS];

    UINT                      edgeLength;
} IWineD3DCubeTextureImpl;

extern const IWineD3DCubeTextureVtbl IWineD3DCubeTexture_Vtbl;

typedef struct _WINED3DVOLUMET_DESC
{
    UINT                    Width;
    UINT                    Height;
    UINT                    Depth;
} WINED3DVOLUMET_DESC;

/*****************************************************************************
 * IWineD3DVolume implementation structure (extends IUnknown)
 */
typedef struct IWineD3DVolumeImpl
{
    /* IUnknown & WineD3DResource fields */
    const IWineD3DVolumeVtbl  *lpVtbl;
    IWineD3DResourceClass      resource;

    /* WineD3DVolume Information */
    WINED3DVOLUMET_DESC      currentDesc;
    IWineD3DBase            *container;
    UINT                    bytesPerPixel;

    BOOL                    lockable;
    BOOL                    locked;
    WINED3DBOX              lockedBox;
    WINED3DBOX              dirtyBox;
    BOOL                    dirty;


} IWineD3DVolumeImpl;

extern const IWineD3DVolumeVtbl IWineD3DVolume_Vtbl;

/*****************************************************************************
 * IWineD3DVolumeTexture implementation structure (extends IWineD3DBaseTextureImpl)
 */
typedef struct IWineD3DVolumeTextureImpl
{
    /* IUnknown & WineD3DResource/WineD3DBaseTexture Information     */
    const IWineD3DVolumeTextureVtbl *lpVtbl;
    IWineD3DResourceClass     resource;
    IWineD3DBaseTextureClass  baseTexture;

    /* IWineD3DVolumeTexture */
    IWineD3DVolume           *volumes[MAX_LEVELS];

    UINT                      width;
    UINT                      height;
    UINT                      depth;
} IWineD3DVolumeTextureImpl;

extern const IWineD3DVolumeTextureVtbl IWineD3DVolumeTexture_Vtbl;

typedef struct _WINED3DSURFACET_DESC
{
    WINED3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD                   MultiSampleQuality;
    UINT                    Width;
    UINT                    Height;
} WINED3DSURFACET_DESC;

/*****************************************************************************
 * Structure for DIB Surfaces (GetDC and GDI surfaces)
 */
typedef struct wineD3DSurface_DIB {
    HBITMAP DIBsection;
    void* bitmap_data;
    UINT bitmap_size;
    HGDIOBJ holdbitmap;
    BOOL client_memory;
} wineD3DSurface_DIB;

typedef struct {
    struct list entry;
    GLuint id;
    UINT width;
    UINT height;
} renderbuffer_entry_t;

/*****************************************************************************
 * IWineD3DClipp implementation structure
 */
typedef struct IWineD3DClipperImpl
{
    const IWineD3DClipperVtbl *lpVtbl;
    LONG ref;

    IUnknown *Parent;
    HWND hWnd;
} IWineD3DClipperImpl;


/*****************************************************************************
 * IWineD3DSurface implementation structure
 */
struct IWineD3DSurfaceImpl
{
    /* IUnknown & IWineD3DResource Information     */
    const IWineD3DSurfaceVtbl *lpVtbl;
    IWineD3DResourceClass     resource;

    /* IWineD3DSurface fields */
    IWineD3DBase              *container;
    WINED3DSURFACET_DESC      currentDesc;
    IWineD3DPaletteImpl       *palette; /* D3D7 style palette handling */
    PALETTEENTRY              *palette9; /* D3D8/9 style palette handling */

    UINT                      bytesPerPixel;

    /* TODO: move this off into a management class(maybe!) */
    DWORD                      Flags;

    UINT                      pow2Width;
    UINT                      pow2Height;

    /* Oversized texture */
    RECT                      glRect;

    /* PBO */
    GLuint                    pbo;

    RECT                      lockedRect;
    RECT                      dirtyRect;
    int                       lockCount;
#define MAXLOCKCOUNT          50 /* After this amount of locks do not free the sysmem copy */

    glDescriptor              glDescription;
    BOOL                      srgb;

    /* For GetDC */
    wineD3DSurface_DIB        dib;
    HDC                       hDC;

    /* Color keys for DDraw */
    WINEDDCOLORKEY            DestBltCKey;
    WINEDDCOLORKEY            DestOverlayCKey;
    WINEDDCOLORKEY            SrcOverlayCKey;
    WINEDDCOLORKEY            SrcBltCKey;
    DWORD                     CKeyFlags;

    WINEDDCOLORKEY            glCKey;

    struct list               renderbuffers;
    renderbuffer_entry_t      *current_renderbuffer;

    /* DirectDraw clippers */
    IWineD3DClipper           *clipper;
};

extern const IWineD3DSurfaceVtbl IWineD3DSurface_Vtbl;
extern const IWineD3DSurfaceVtbl IWineGDISurface_Vtbl;

/* Predeclare the shared Surface functions */
HRESULT WINAPI IWineD3DBaseSurfaceImpl_QueryInterface(IWineD3DSurface *iface, REFIID riid, LPVOID *ppobj);
ULONG WINAPI IWineD3DBaseSurfaceImpl_AddRef(IWineD3DSurface *iface);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetParent(IWineD3DSurface *iface, IUnknown **pParent);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetDevice(IWineD3DSurface *iface, IWineD3DDevice** ppDevice);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetPrivateData(IWineD3DSurface *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetPrivateData(IWineD3DSurface *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_FreePrivateData(IWineD3DSurface *iface, REFGUID refguid);
DWORD   WINAPI IWineD3DBaseSurfaceImpl_SetPriority(IWineD3DSurface *iface, DWORD PriorityNew);
DWORD   WINAPI IWineD3DBaseSurfaceImpl_GetPriority(IWineD3DSurface *iface);
WINED3DRESOURCETYPE WINAPI IWineD3DBaseSurfaceImpl_GetType(IWineD3DSurface *iface);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetContainer(IWineD3DSurface* iface, REFIID riid, void** ppContainer);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetDesc(IWineD3DSurface *iface, WINED3DSURFACE_DESC *pDesc);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetBltStatus(IWineD3DSurface *iface, DWORD Flags);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetFlipStatus(IWineD3DSurface *iface, DWORD Flags);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_IsLost(IWineD3DSurface *iface);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_Restore(IWineD3DSurface *iface);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetPalette(IWineD3DSurface *iface, IWineD3DPalette **Pal);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetPalette(IWineD3DSurface *iface, IWineD3DPalette *Pal);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetColorKey(IWineD3DSurface *iface, DWORD Flags, WINEDDCOLORKEY *CKey);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetContainer(IWineD3DSurface *iface, IWineD3DBase *container);
DWORD WINAPI IWineD3DBaseSurfaceImpl_GetPitch(IWineD3DSurface *iface);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_RealizePalette(IWineD3DSurface *iface);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetOverlayPosition(IWineD3DSurface *iface, LONG X, LONG Y);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetOverlayPosition(IWineD3DSurface *iface, LONG *X, LONG *Y);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_UpdateOverlayZOrder(IWineD3DSurface *iface, DWORD Flags, IWineD3DSurface *Ref);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_UpdateOverlay(IWineD3DSurface *iface, RECT *SrcRect, IWineD3DSurface *DstSurface, RECT *DstRect, DWORD Flags, WINEDDOVERLAYFX *FX);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetClipper(IWineD3DSurface *iface, IWineD3DClipper *clipper);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetClipper(IWineD3DSurface *iface, IWineD3DClipper **clipper);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetFormat(IWineD3DSurface *iface, WINED3DFORMAT format);
HRESULT IWineD3DBaseSurfaceImpl_CreateDIBSection(IWineD3DSurface *iface);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_Blt(IWineD3DSurface *iface, RECT *DestRect, IWineD3DSurface *SrcSurface, RECT *SrcRect, DWORD Flags, WINEDDBLTFX *DDBltFx, WINED3DTEXTUREFILTERTYPE Filter);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_BltFast(IWineD3DSurface *iface, DWORD dstx, DWORD dsty, IWineD3DSurface *Source, RECT *rsrc, DWORD trans);
HRESULT WINAPI IWineD3DBaseSurfaceImpl_LockRect(IWineD3DSurface *iface, WINED3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags);
void WINAPI IWineD3DBaseSurfaceImpl_BindTexture(IWineD3DSurface *iface);

const void *WINAPI IWineD3DSurfaceImpl_GetData(IWineD3DSurface *iface);

/* Surface flags: */
#define SFLAG_OVERSIZE    0x00000001 /* Surface is bigger than gl size, blts only */
#define SFLAG_CONVERTED   0x00000002 /* Converted for color keying or Palettized */
#define SFLAG_DIBSECTION  0x00000004 /* Has a DIB section attached for getdc */
#define SFLAG_LOCKABLE    0x00000008 /* Surface can be locked */
#define SFLAG_DISCARD     0x00000010 /* ??? */
#define SFLAG_LOCKED      0x00000020 /* Surface is locked atm */
#define SFLAG_INTEXTURE   0x00000040 /* The GL texture contains the newest surface content */
#define SFLAG_INDRAWABLE  0x00000080 /* The gl drawable contains the most up to date data */
#define SFLAG_INSYSMEM    0x00000100 /* The system memory copy is most up to date */
#define SFLAG_NONPOW2     0x00000200 /* Surface sizes are not a power of 2 */
#define SFLAG_DYNLOCK     0x00000400 /* Surface is often locked by the app */
#define SFLAG_DYNCHANGE   0x00000C00 /* Surface contents are changed very often, implies DYNLOCK */
#define SFLAG_DCINUSE     0x00001000 /* Set between GetDC and ReleaseDC calls */
#define SFLAG_LOST        0x00002000 /* Surface lost flag for DDraw */
#define SFLAG_USERPTR     0x00004000 /* The application allocated the memory for this surface */
#define SFLAG_GLCKEY      0x00008000 /* The gl texture was created with a color key */
#define SFLAG_CLIENT      0x00010000 /* GL_APPLE_client_storage is used on that texture */
#define SFLAG_ALLOCATED   0x00020000 /* A gl texture is allocated for this surface */
#define SFLAG_PBO         0x00040000 /* Has a PBO attached for speeding up data transfers for dynamically locked surfaces */

/* In some conditions the surface memory must not be freed:
 * SFLAG_OVERSIZE: Not all data can be kept in GL
 * SFLAG_CONVERTED: Converting the data back would take too long
 * SFLAG_DIBSECTION: The dib code manages the memory
 * SFLAG_LOCKED: The app requires access to the surface data
 * SFLAG_DYNLOCK: Avoid freeing the data for performance
 * SFLAG_DYNCHANGE: Same reason as DYNLOCK
 * SFLAG_PBO: PBOs don't use 'normal' memory. It is either allocated by the driver or must be NULL.
 * SFLAG_CLIENT: OpenGL uses our memory as backup
 */
#define SFLAG_DONOTFREE  (SFLAG_OVERSIZE   | \
                          SFLAG_CONVERTED  | \
                          SFLAG_DIBSECTION | \
                          SFLAG_LOCKED     | \
                          SFLAG_DYNLOCK    | \
                          SFLAG_DYNCHANGE  | \
                          SFLAG_USERPTR    | \
                          SFLAG_PBO        | \
                          SFLAG_CLIENT)

#define SFLAG_LOCATIONS  (SFLAG_INSYSMEM   | \
                          SFLAG_INTEXTURE  | \
                          SFLAG_INDRAWABLE)
BOOL CalculateTexRect(IWineD3DSurfaceImpl *This, RECT *Rect, float glTexCoord[4]);

typedef enum {
    NO_CONVERSION,
    CONVERT_PALETTED,
    CONVERT_PALETTED_CK,
    CONVERT_CK_565,
    CONVERT_CK_5551,
    CONVERT_CK_4444,
    CONVERT_CK_4444_ARGB,
    CONVERT_CK_1555,
    CONVERT_555,
    CONVERT_CK_RGB24,
    CONVERT_CK_8888,
    CONVERT_CK_8888_ARGB,
    CONVERT_RGB32_888,
    CONVERT_V8U8,
    CONVERT_L6V5U5,
    CONVERT_X8L8V8U8,
    CONVERT_Q8W8V8U8,
    CONVERT_V16U16,
    CONVERT_A4L4,
    CONVERT_R32F,
    CONVERT_R16F
} CONVERT_TYPES;

HRESULT d3dfmt_get_conv(IWineD3DSurfaceImpl *This, BOOL need_alpha_ck, BOOL use_texturing, GLenum *format, GLenum *internal, GLenum *type, CONVERT_TYPES *convert, int *target_bpp, BOOL srgb_mode);

/*****************************************************************************
 * IWineD3DVertexDeclaration implementation structure
 */
typedef struct attrib_declaration {
    DWORD usage;
    DWORD idx;
} attrib_declaration;

#define MAX_ATTRIBS 16

typedef struct IWineD3DVertexDeclarationImpl {
    /* IUnknown  Information */
    const IWineD3DVertexDeclarationVtbl *lpVtbl;
    LONG                    ref;

    IUnknown                *parent;
    IWineD3DDeviceImpl      *wineD3DDevice;

    WINED3DVERTEXELEMENT    *pDeclarationWine;
    UINT                    declarationWNumElements;

    DWORD                   streams[MAX_STREAMS];
    UINT                    num_streams;
    BOOL                    position_transformed;

    /* Ordered array of declaration types that need swizzling in a vshader */
    attrib_declaration      swizzled_attribs[MAX_ATTRIBS];
    UINT                    num_swizzled_attribs;
} IWineD3DVertexDeclarationImpl;

extern const IWineD3DVertexDeclarationVtbl IWineD3DVertexDeclaration_Vtbl;

/*****************************************************************************
 * IWineD3DStateBlock implementation structure
 */

/* Internal state Block for Begin/End/Capture/Create/Apply info  */
/*   Note: Very long winded but gl Lists are not flexible enough */
/*   to resolve everything we need, so doing it manually for now */
typedef struct SAVEDSTATES {
        BOOL                      indices;
        BOOL                      material;
        BOOL                      fvf;
        BOOL                      streamSource[MAX_STREAMS];
        BOOL                      streamFreq[MAX_STREAMS];
        BOOL                      textures[MAX_COMBINED_SAMPLERS];
        BOOL                      transform[HIGHEST_TRANSFORMSTATE + 1];
        BOOL                      viewport;
        BOOL                      renderState[WINEHIGHEST_RENDER_STATE + 1];
        BOOL                      textureState[MAX_TEXTURES][WINED3D_HIGHEST_TEXTURE_STATE + 1];
        BOOL                      samplerState[MAX_COMBINED_SAMPLERS][WINED3D_HIGHEST_SAMPLER_STATE + 1];
        BOOL                      clipplane[MAX_CLIPPLANES];
        BOOL                      vertexDecl;
        BOOL                      pixelShader;
        BOOL                      pixelShaderConstantsB[MAX_CONST_B];
        BOOL                      pixelShaderConstantsI[MAX_CONST_I];
        BOOL                     *pixelShaderConstantsF;
        BOOL                      vertexShader;
        BOOL                      vertexShaderConstantsB[MAX_CONST_B];
        BOOL                      vertexShaderConstantsI[MAX_CONST_I];
        BOOL                     *vertexShaderConstantsF;
        BOOL                      scissorRect;
} SAVEDSTATES;

typedef struct {
    struct  list entry;
    DWORD   count;
    DWORD   idx[13];
} constants_entry;

struct StageState {
    DWORD stage;
    DWORD state;
};

struct IWineD3DStateBlockImpl
{
    /* IUnknown fields */
    const IWineD3DStateBlockVtbl *lpVtbl;
    LONG                      ref;     /* Note: Ref counting not required */

    /* IWineD3DStateBlock information */
    IUnknown                 *parent;
    IWineD3DDeviceImpl       *wineD3DDevice;
    WINED3DSTATEBLOCKTYPE     blockType;

    /* Array indicating whether things have been set or changed */
    SAVEDSTATES               changed;
    struct list               set_vconstantsF;
    struct list               set_pconstantsF;

    /* Drawing - Vertex Shader or FVF related */
    DWORD                     fvf;
    /* Vertex Shader Declaration */
    IWineD3DVertexDeclaration *vertexDecl;

    IWineD3DVertexShader      *vertexShader;

    /* Vertex Shader Constants */
    BOOL                       vertexShaderConstantB[MAX_CONST_B];
    INT                        vertexShaderConstantI[MAX_CONST_I * 4];
    float                     *vertexShaderConstantF;

    /* Stream Source */
    BOOL                      streamIsUP;
    UINT                      streamStride[MAX_STREAMS];
    UINT                      streamOffset[MAX_STREAMS + 1 /* tesselated pseudo-stream */ ];
    IWineD3DVertexBuffer     *streamSource[MAX_STREAMS];
    UINT                      streamFreq[MAX_STREAMS + 1];
    UINT                      streamFlags[MAX_STREAMS + 1];     /*0 | WINED3DSTREAMSOURCE_INSTANCEDATA | WINED3DSTREAMSOURCE_INDEXEDDATA  */

    /* Indices */
    IWineD3DIndexBuffer*      pIndexData;
    INT                       baseVertexIndex;
    INT                       loadBaseVertexIndex; /* non-indexed drawing needs 0 here, indexed baseVertexIndex */

    /* Transform */
    WINED3DMATRIX             transforms[HIGHEST_TRANSFORMSTATE + 1];

    /* Light hashmap . Collisions are handled using standard wine double linked lists */
#define LIGHTMAP_SIZE 43 /* Use of a prime number recommended. Set to 1 for a linked list! */
#define LIGHTMAP_HASHFUNC(x) ((x) % LIGHTMAP_SIZE) /* Primitive and simple function */
    struct list               lightMap[LIGHTMAP_SIZE]; /* Mashmap containing the lights */
    PLIGHTINFOEL             *activeLights[MAX_ACTIVE_LIGHTS]; /* Map of opengl lights to d3d lights */

    /* Clipping */
    double                    clipplane[MAX_CLIPPLANES][4];
    WINED3DCLIPSTATUS         clip_status;

    /* ViewPort */
    WINED3DVIEWPORT           viewport;

    /* Material */
    WINED3DMATERIAL           material;

    /* Pixel Shader */
    IWineD3DPixelShader      *pixelShader;

    /* Pixel Shader Constants */
    BOOL                       pixelShaderConstantB[MAX_CONST_B];
    INT                        pixelShaderConstantI[MAX_CONST_I * 4];
    float                     *pixelShaderConstantF;

    /* RenderState */
    DWORD                     renderState[WINEHIGHEST_RENDER_STATE + 1];

    /* Texture */
    IWineD3DBaseTexture      *textures[MAX_COMBINED_SAMPLERS];
    int                       textureDimensions[MAX_COMBINED_SAMPLERS];

    /* Texture State Stage */
    DWORD                     textureState[MAX_TEXTURES][WINED3D_HIGHEST_TEXTURE_STATE + 1];
    DWORD                     lowest_disabled_stage;
    /* Sampler States */
    DWORD                     samplerState[MAX_COMBINED_SAMPLERS][WINED3D_HIGHEST_SAMPLER_STATE + 1];

    /* Current GLSL Shader Program */
    struct glsl_shader_prog_link *glsl_program;

    /* Scissor test rectangle */
    RECT                      scissorRect;

    /* Contained state management */
    DWORD                     contained_render_states[WINEHIGHEST_RENDER_STATE + 1];
    unsigned int              num_contained_render_states;
    DWORD                     contained_transform_states[HIGHEST_TRANSFORMSTATE + 1];
    unsigned int              num_contained_transform_states;
    DWORD                     contained_vs_consts_i[MAX_CONST_I];
    unsigned int              num_contained_vs_consts_i;
    DWORD                     contained_vs_consts_b[MAX_CONST_B];
    unsigned int              num_contained_vs_consts_b;
    DWORD                     *contained_vs_consts_f;
    unsigned int              num_contained_vs_consts_f;
    DWORD                     contained_ps_consts_i[MAX_CONST_I];
    unsigned int              num_contained_ps_consts_i;
    DWORD                     contained_ps_consts_b[MAX_CONST_B];
    unsigned int              num_contained_ps_consts_b;
    DWORD                     *contained_ps_consts_f;
    unsigned int              num_contained_ps_consts_f;
    struct StageState         contained_tss_states[MAX_TEXTURES * (WINED3D_HIGHEST_TEXTURE_STATE)];
    unsigned int              num_contained_tss_states;
    struct StageState         contained_sampler_states[MAX_COMBINED_SAMPLERS * WINED3D_HIGHEST_SAMPLER_STATE];
    unsigned int              num_contained_sampler_states;
};

extern void stateblock_savedstates_set(
    IWineD3DStateBlock* iface,
    SAVEDSTATES* states,
    BOOL value);

extern void stateblock_savedstates_copy(
    IWineD3DStateBlock* iface,
    SAVEDSTATES* dest,
    SAVEDSTATES* source);

extern void stateblock_copy(
    IWineD3DStateBlock* destination,
    IWineD3DStateBlock* source);

extern const IWineD3DStateBlockVtbl IWineD3DStateBlock_Vtbl;

/*****************************************************************************
 * IWineD3DQueryImpl implementation structure (extends IUnknown)
 */
typedef struct IWineD3DQueryImpl
{
    const IWineD3DQueryVtbl  *lpVtbl;
    LONG                      ref;     /* Note: Ref counting not required */
    
    IUnknown                 *parent;
    /*TODO: replace with iface usage */
#if 0
    IWineD3DDevice         *wineD3DDevice;
#else
    IWineD3DDeviceImpl       *wineD3DDevice;
#endif

    /* IWineD3DQuery fields */
    WINED3DQUERYTYPE         type;
    /* TODO: Think about using a IUnknown instead of a void* */
    void                     *extendedData;
    
  
} IWineD3DQueryImpl;

extern const IWineD3DQueryVtbl IWineD3DQuery_Vtbl;

/* Datastructures for IWineD3DQueryImpl.extendedData */
typedef struct  WineQueryOcclusionData {
    GLuint  queryId;
    WineD3DContext *ctx;
} WineQueryOcclusionData;

typedef struct  WineQueryEventData {
    GLuint  fenceId;
    WineD3DContext *ctx;
} WineQueryEventData;

/*****************************************************************************
 * IWineD3DSwapChainImpl implementation structure (extends IUnknown)
 */

typedef struct IWineD3DSwapChainImpl
{
    /*IUnknown part*/
    const IWineD3DSwapChainVtbl *lpVtbl;
    LONG                      ref;     /* Note: Ref counting not required */

    IUnknown                 *parent;
    IWineD3DDeviceImpl       *wineD3DDevice;

    /* IWineD3DSwapChain fields */
    IWineD3DSurface         **backBuffer;
    IWineD3DSurface          *frontBuffer;
    BOOL                      wantsDepthStencilBuffer;
    WINED3DPRESENT_PARAMETERS presentParms;
    DWORD                     orig_width, orig_height;
    WINED3DFORMAT             orig_fmt;

    long prev_time, frames;   /* Performance tracking */
    unsigned int vSyncCounter;

    WineD3DContext        **context; /* Later a array for multithreading */
    unsigned int            num_contexts;

    HWND                    win_handle;
} IWineD3DSwapChainImpl;

extern const IWineD3DSwapChainVtbl IWineD3DSwapChain_Vtbl;

WineD3DContext *IWineD3DSwapChainImpl_CreateContextForThread(IWineD3DSwapChain *iface);

/*****************************************************************************
 * Utility function prototypes 
 */

/* Trace routines */
const char* debug_d3dformat(WINED3DFORMAT fmt);
const char* debug_d3ddevicetype(WINED3DDEVTYPE devtype);
const char* debug_d3dresourcetype(WINED3DRESOURCETYPE res);
const char* debug_d3dusage(DWORD usage);
const char* debug_d3dusagequery(DWORD usagequery);
const char* debug_d3ddeclmethod(WINED3DDECLMETHOD method);
const char* debug_d3ddecltype(WINED3DDECLTYPE type);
const char* debug_d3ddeclusage(BYTE usage);
const char* debug_d3dprimitivetype(WINED3DPRIMITIVETYPE PrimitiveType);
const char* debug_d3drenderstate(DWORD state);
const char* debug_d3dsamplerstate(DWORD state);
const char* debug_d3dtexturefiltertype(WINED3DTEXTUREFILTERTYPE filter_type);
const char* debug_d3dtexturestate(DWORD state);
const char* debug_d3dtstype(WINED3DTRANSFORMSTATETYPE tstype);
const char* debug_d3dpool(WINED3DPOOL pool);
const char *debug_fbostatus(GLenum status);
const char *debug_glerror(GLenum error);
const char *debug_d3dbasis(WINED3DBASISTYPE basis);
const char *debug_d3ddegree(WINED3DDEGREETYPE order);

/* Routines for GL <-> D3D values */
GLenum StencilOp(DWORD op);
GLenum CompareFunc(DWORD func);
void   set_tex_op(IWineD3DDevice *iface, BOOL isAlpha, int Stage, WINED3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3);
void   set_tex_op_nvrc(IWineD3DDevice *iface, BOOL is_alpha, int stage, WINED3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3, INT texture_idx);
void   set_texture_matrix(const float *smat, DWORD flags, BOOL calculatedCoords, BOOL transformed, DWORD coordtype);

void surface_set_compatible_renderbuffer(IWineD3DSurface *iface, unsigned int width, unsigned int height);
GLenum surface_get_gl_buffer(IWineD3DSurface *iface, IWineD3DSwapChain *swapchain);

BOOL getColorBits(WINED3DFORMAT fmt, short *redSize, short *greenSize, short *blueSize, short *alphaSize, short *totalSize);
BOOL getDepthStencilBits(WINED3DFORMAT fmt, short *depthSize, short *stencilSize);

/* Math utils */
void multiply_matrix(WINED3DMATRIX *dest, const WINED3DMATRIX *src1, const WINED3DMATRIX *src2);
unsigned int count_bits(unsigned int mask);

/*****************************************************************************
 * To enable calling of inherited functions, requires prototypes 
 *
 * Note: Only require classes which are subclassed, ie resource, basetexture, 
 */
    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DResourceImpl_QueryInterface(IWineD3DResource *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DResourceImpl_AddRef(IWineD3DResource *iface);
    extern ULONG WINAPI IWineD3DResourceImpl_Release(IWineD3DResource *iface);
    /*** IWineD3DResource methods ***/
    extern HRESULT WINAPI IWineD3DResourceImpl_GetParent(IWineD3DResource *iface, IUnknown **pParent);
    extern HRESULT WINAPI IWineD3DResourceImpl_GetDevice(IWineD3DResource *iface, IWineD3DDevice ** ppDevice);
    extern HRESULT WINAPI IWineD3DResourceImpl_SetPrivateData(IWineD3DResource *iface, REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags);
    extern HRESULT WINAPI IWineD3DResourceImpl_GetPrivateData(IWineD3DResource *iface, REFGUID  refguid, void * pData, DWORD * pSizeOfData);
    extern HRESULT WINAPI IWineD3DResourceImpl_FreePrivateData(IWineD3DResource *iface, REFGUID  refguid);
    extern DWORD WINAPI IWineD3DResourceImpl_SetPriority(IWineD3DResource *iface, DWORD  PriorityNew);
    extern DWORD WINAPI IWineD3DResourceImpl_GetPriority(IWineD3DResource *iface);
    extern void WINAPI IWineD3DResourceImpl_PreLoad(IWineD3DResource *iface);
    extern WINED3DRESOURCETYPE WINAPI IWineD3DResourceImpl_GetType(IWineD3DResource *iface);
    /*** class static members ***/
    void IWineD3DResourceImpl_CleanUp(IWineD3DResource *iface);

    /*** IUnknown methods ***/
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_QueryInterface(IWineD3DBaseTexture *iface, REFIID riid, void** ppvObject);
    extern ULONG WINAPI IWineD3DBaseTextureImpl_AddRef(IWineD3DBaseTexture *iface);
    extern ULONG WINAPI IWineD3DBaseTextureImpl_Release(IWineD3DBaseTexture *iface);
    /*** IWineD3DResource methods ***/
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_GetParent(IWineD3DBaseTexture *iface, IUnknown **pParent);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_GetDevice(IWineD3DBaseTexture *iface, IWineD3DDevice ** ppDevice);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_SetPrivateData(IWineD3DBaseTexture *iface, REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_GetPrivateData(IWineD3DBaseTexture *iface, REFGUID  refguid, void * pData, DWORD * pSizeOfData);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_FreePrivateData(IWineD3DBaseTexture *iface, REFGUID  refguid);
    extern DWORD WINAPI IWineD3DBaseTextureImpl_SetPriority(IWineD3DBaseTexture *iface, DWORD  PriorityNew);
    extern DWORD WINAPI IWineD3DBaseTextureImpl_GetPriority(IWineD3DBaseTexture *iface);
    extern void WINAPI IWineD3DBaseTextureImpl_PreLoad(IWineD3DBaseTexture *iface);
    extern WINED3DRESOURCETYPE WINAPI IWineD3DBaseTextureImpl_GetType(IWineD3DBaseTexture *iface);
    /*** IWineD3DBaseTexture methods ***/
    extern DWORD WINAPI IWineD3DBaseTextureImpl_SetLOD(IWineD3DBaseTexture *iface, DWORD LODNew);
    extern DWORD WINAPI IWineD3DBaseTextureImpl_GetLOD(IWineD3DBaseTexture *iface);
    extern DWORD WINAPI IWineD3DBaseTextureImpl_GetLevelCount(IWineD3DBaseTexture *iface);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_SetAutoGenFilterType(IWineD3DBaseTexture *iface, WINED3DTEXTUREFILTERTYPE FilterType);
    extern WINED3DTEXTUREFILTERTYPE WINAPI IWineD3DBaseTextureImpl_GetAutoGenFilterType(IWineD3DBaseTexture *iface);
    extern void WINAPI IWineD3DBaseTextureImpl_GenerateMipSubLevels(IWineD3DBaseTexture *iface);
    extern BOOL WINAPI IWineD3DBaseTextureImpl_SetDirty(IWineD3DBaseTexture *iface, BOOL);
    extern BOOL WINAPI IWineD3DBaseTextureImpl_GetDirty(IWineD3DBaseTexture *iface);

    extern BYTE* WINAPI IWineD3DVertexBufferImpl_GetMemory(IWineD3DVertexBuffer* iface, DWORD iOffset, GLint *vbo);
    extern HRESULT WINAPI IWineD3DVertexBufferImpl_ReleaseMemory(IWineD3DVertexBuffer* iface);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_BindTexture(IWineD3DBaseTexture *iface);
    extern HRESULT WINAPI IWineD3DBaseTextureImpl_UnBindTexture(IWineD3DBaseTexture *iface);
    extern void WINAPI IWineD3DBaseTextureImpl_ApplyStateChanges(IWineD3DBaseTexture *iface, const DWORD textureStates[WINED3D_HIGHEST_TEXTURE_STATE + 1], const DWORD samplerStates[WINED3D_HIGHEST_SAMPLER_STATE + 1]);
    /*** class static members ***/
    void IWineD3DBaseTextureImpl_CleanUp(IWineD3DBaseTexture *iface);

typedef void (*SHADER_HANDLER) (struct SHADER_OPCODE_ARG*);

/* Struct to maintain a list of GLSL shader programs and their associated pixel and
 * vertex shaders.  A list of this type is maintained on the DeviceImpl, and is only
 * used if the user is using GLSL shaders. */
struct glsl_shader_prog_link {
    struct list             vshader_entry;
    struct list             pshader_entry;
    GLhandleARB             programId;
    GLhandleARB             *vuniformF_locations;
    GLhandleARB             *puniformF_locations;
    GLhandleARB             vuniformI_locations[MAX_CONST_I];
    GLhandleARB             puniformI_locations[MAX_CONST_I];
    GLhandleARB             posFixup_location;
    GLhandleARB             bumpenvmat_location;
    GLhandleARB             luminancescale_location;
    GLhandleARB             luminanceoffset_location;
    GLhandleARB             srgb_comparison_location;
    GLhandleARB             srgb_mul_low_location;
    GLhandleARB             ycorrection_location;
    GLhandleARB             vshader;
    GLhandleARB             pshader;
};

typedef struct {
    GLhandleARB vshader;
    GLhandleARB pshader;
} glsl_program_key_t;

/* TODO: Make this dynamic, based on shader limits ? */
#define MAX_REG_ADDR 1
#define MAX_REG_TEMP 32
#define MAX_REG_TEXCRD 8
#define MAX_REG_INPUT 12
#define MAX_REG_OUTPUT 12
#define MAX_CONST_I 16
#define MAX_CONST_B 16

/* FIXME: This needs to go up to 2048 for
 * Shader model 3 according to msdn (and for software shaders) */
#define MAX_LABELS 16

typedef struct semantic {
    DWORD usage;
    DWORD reg;
} semantic;

typedef struct local_constant {
    struct list entry;
    unsigned int idx;
    DWORD value[4];
} local_constant;

typedef struct shader_reg_maps {

    char texcoord[MAX_REG_TEXCRD];          /* pixel < 3.0 */
    char temporary[MAX_REG_TEMP];           /* pixel, vertex */
    char address[MAX_REG_ADDR];             /* vertex */
    char packed_input[MAX_REG_INPUT];       /* pshader >= 3.0 */
    char packed_output[MAX_REG_OUTPUT];     /* vertex >= 3.0 */
    char attributes[MAX_ATTRIBS];           /* vertex */
    char labels[MAX_LABELS];                /* pixel, vertex */

    /* Sampler usage tokens 
     * Use 0 as default (bit 31 is always 1 on a valid token) */
    DWORD samplers[max(MAX_FRAGMENT_SAMPLERS, MAX_VERTEX_SAMPLERS)];
    char bumpmat, luminanceparams;
    char usesnrm, vpos, usesdsy;
    char usesrelconstF;

    /* Whether or not loops are used in this shader, and nesting depth */
    unsigned loop_depth;

    /* Whether or not this shader uses fog */
    char fog;

} shader_reg_maps;

#define SHADER_PGMSIZE 65535
typedef struct SHADER_BUFFER {
    char* buffer;
    unsigned int bsize;
    unsigned int lineNo;
    BOOL newline;
} SHADER_BUFFER;

/* Undocumented opcode controls */
#define INST_CONTROLS_SHIFT 16
#define INST_CONTROLS_MASK 0x00ff0000

typedef enum COMPARISON_TYPE {
    COMPARISON_GT = 1,
    COMPARISON_EQ = 2,
    COMPARISON_GE = 3,
    COMPARISON_LT = 4,
    COMPARISON_NE = 5,
    COMPARISON_LE = 6
} COMPARISON_TYPE;

typedef struct SHADER_OPCODE {
    unsigned int  opcode;
    const char*   name;
    const char*   glname;
    char          dst_token;
    CONST UINT    num_params;
    SHADER_HANDLER hw_fct;
    SHADER_HANDLER hw_glsl_fct;
    DWORD         min_version;
    DWORD         max_version;
} SHADER_OPCODE;

typedef struct SHADER_OPCODE_ARG {
    IWineD3DBaseShader* shader;
    shader_reg_maps* reg_maps;
    CONST SHADER_OPCODE* opcode;
    DWORD opcode_token;
    DWORD dst;
    DWORD dst_addr;
    DWORD predicate;
    DWORD src[4];
    DWORD src_addr[4];
    SHADER_BUFFER* buffer;
} SHADER_OPCODE_ARG;

typedef struct SHADER_LIMITS {
    unsigned int temporary;
    unsigned int texcoord;
    unsigned int sampler;
    unsigned int constant_int;
    unsigned int constant_float;
    unsigned int constant_bool;
    unsigned int address;
    unsigned int packed_output;
    unsigned int packed_input;
    unsigned int attributes;
    unsigned int label;
} SHADER_LIMITS;

/** Keeps track of details for TEX_M#x# shader opcodes which need to 
    maintain state information between multiple codes */
typedef struct SHADER_PARSE_STATE {
    unsigned int current_row;
    DWORD texcoord_w[2];
} SHADER_PARSE_STATE;

/* Base Shader utility functions. 
 * (may move callers into the same file in the future) */
extern int shader_addline(
    SHADER_BUFFER* buffer,
    const char* fmt, ...);

extern const SHADER_OPCODE* shader_get_opcode(
    IWineD3DBaseShader *iface, 
    const DWORD code);

void delete_glsl_program_entry(IWineD3DDevice *iface, struct glsl_shader_prog_link *entry);

/* Vertex shader utility functions */
extern BOOL vshader_get_input(
    IWineD3DVertexShader* iface,
    BYTE usage_req, BYTE usage_idx_req,
    unsigned int* regnum);

extern BOOL vshader_input_is_color(
    IWineD3DVertexShader* iface,
    unsigned int regnum);

extern HRESULT allocate_shader_constants(IWineD3DStateBlockImpl* object);

/* ARB_[vertex/fragment]_program helper functions */
extern void shader_arb_load_constants(
    IWineD3DDevice* device,
    char usePixelShader,
    char useVertexShader);

/* ARB shader program Prototypes */
extern void shader_hw_def(SHADER_OPCODE_ARG *arg);

/* ARB pixel shader prototypes */
extern void pshader_hw_bem(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_cnd(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_cmp(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_map2gl(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_tex(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texcoord(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texreg2ar(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texreg2gb(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texbem(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texm3x2pad(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texm3x2tex(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texm3x3pad(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texm3x3tex(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texm3x3spec(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texm3x3vspec(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texdepth(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texkill(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texdp3tex(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texdp3(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texm3x3(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texm3x2depth(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_dp2add(SHADER_OPCODE_ARG* arg);
extern void pshader_hw_texreg2rgb(SHADER_OPCODE_ARG* arg);

/* ARB vertex / pixel shader common prototypes */
extern void shader_hw_nrm(SHADER_OPCODE_ARG* arg);
extern void shader_hw_sincos(SHADER_OPCODE_ARG* arg);
extern void shader_hw_mnxn(SHADER_OPCODE_ARG* arg);

/* ARB vertex shader prototypes */
extern void vshader_hw_map2gl(SHADER_OPCODE_ARG* arg);
extern void vshader_hw_rsq_rcp(SHADER_OPCODE_ARG* arg);

/* GLSL helper functions */
extern void shader_glsl_add_instruction_modifiers(SHADER_OPCODE_ARG *arg);
extern void shader_glsl_load_constants(
    IWineD3DDevice* device,
    char usePixelShader,
    char useVertexShader);

/** The following translate DirectX pixel/vertex shader opcodes to GLSL lines */
extern void shader_glsl_cross(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_map2gl(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_arith(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_mov(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_mad(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_mnxn(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_lrp(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_dot(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_rcp(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_rsq(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_cnd(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_compare(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_def(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_defi(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_defb(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_expp(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_cmp(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_lit(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_dst(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_sincos(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_loop(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_end(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_if(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_ifc(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_else(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_break(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_breakc(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_rep(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_call(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_callnz(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_label(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_pow(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_log(SHADER_OPCODE_ARG* arg);
extern void shader_glsl_texldl(SHADER_OPCODE_ARG* arg);

/** GLSL Pixel Shader Prototypes */
extern void pshader_glsl_tex(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texcoord(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texdp3tex(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texdp3(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texdepth(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texm3x2depth(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texm3x2pad(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texm3x2tex(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texm3x3(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texm3x3pad(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texm3x3tex(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texm3x3spec(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texm3x3vspec(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texkill(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texbem(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_bem(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texreg2ar(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texreg2gb(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_texreg2rgb(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_dp2add(SHADER_OPCODE_ARG* arg);
extern void pshader_glsl_input_pack(
   SHADER_BUFFER* buffer,
   semantic* semantics_out,
   IWineD3DPixelShader *iface);

/*****************************************************************************
 * IDirect3DBaseShader implementation structure
 */
typedef struct IWineD3DBaseShaderClass
{
    LONG                            ref;
    DWORD                           hex_version;
    SHADER_LIMITS                   limits;
    SHADER_PARSE_STATE              parse_state;
    CONST SHADER_OPCODE             *shader_ins;
    DWORD                          *function;
    UINT                            functionLength;
    GLuint                          prgId;
    BOOL                            is_compiled;
    UINT                            cur_loop_depth, cur_loop_regno;
    BOOL                            load_local_constsF;

    /* Type of shader backend */
    int shader_mode;

    /* Programs this shader is linked with */
    struct list linked_programs;

    /* Immediate constants (override global ones) */
    struct list constantsB;
    struct list constantsF;
    struct list constantsI;
    shader_reg_maps reg_maps;

    /* Pixel formats of sampled textures, for format conversion. This
     * represents the formats found during compilation, it is not initialized
     * on the first parser pass. It is needed to check if the shader
     * needs recompilation to adjust the format conversion
     */
    WINED3DFORMAT       sampled_format[MAX_COMBINED_SAMPLERS];
    UINT                sampled_samplers[MAX_COMBINED_SAMPLERS];
    UINT                num_sampled_samplers;

    UINT recompile_count;

    /* Pointer to the parent device */
    IWineD3DDevice *device;

} IWineD3DBaseShaderClass;

typedef struct IWineD3DBaseShaderImpl {
    /* IUnknown */
    const IWineD3DBaseShaderVtbl    *lpVtbl;

    /* IWineD3DBaseShader */
    IWineD3DBaseShaderClass         baseShader;
} IWineD3DBaseShaderImpl;

HRESULT  WINAPI IWineD3DBaseShaderImpl_QueryInterface(IWineD3DBaseShader *iface, REFIID riid, LPVOID *ppobj);
ULONG  WINAPI IWineD3DBaseShaderImpl_AddRef(IWineD3DBaseShader *iface);
ULONG  WINAPI IWineD3DBaseShaderImpl_Release(IWineD3DBaseShader *iface);

extern HRESULT shader_get_registers_used(
    IWineD3DBaseShader *iface,
    shader_reg_maps* reg_maps,
    semantic* semantics_in,
    semantic* semantics_out,
    CONST DWORD* pToken,
    IWineD3DStateBlockImpl *stateBlock);

extern void shader_generate_glsl_declarations(
    IWineD3DBaseShader *iface,
    shader_reg_maps* reg_maps,
    SHADER_BUFFER* buffer,
    WineD3D_GL_Info* gl_info);

extern void shader_generate_arb_declarations(
    IWineD3DBaseShader *iface,
    shader_reg_maps* reg_maps,
    SHADER_BUFFER* buffer,
    WineD3D_GL_Info* gl_info);

extern void shader_generate_main(
    IWineD3DBaseShader *iface,
    SHADER_BUFFER* buffer,
    shader_reg_maps* reg_maps,
    CONST DWORD* pFunction);

extern void shader_dump_ins_modifiers(
    const DWORD output);

extern void shader_dump_param(
    IWineD3DBaseShader *iface,
    const DWORD param,
    const DWORD addr_token,
    int input);

extern void shader_trace_init(
    IWineD3DBaseShader *iface,
    const DWORD* pFunction);

extern int shader_get_param(
    IWineD3DBaseShader* iface,
    const DWORD* pToken,
    DWORD* param,
    DWORD* addr_token);

extern int shader_skip_unrecognized(
    IWineD3DBaseShader* iface,
    const DWORD* pToken);

extern void print_glsl_info_log(
    WineD3D_GL_Info *gl_info,
    GLhandleARB obj);

static inline int shader_get_regtype(const DWORD param) {
    return (((param & WINED3DSP_REGTYPE_MASK) >> WINED3DSP_REGTYPE_SHIFT) |
            ((param & WINED3DSP_REGTYPE_MASK2) >> WINED3DSP_REGTYPE_SHIFT2));
}

extern unsigned int shader_get_float_offset(const DWORD reg);

static inline BOOL shader_is_pshader_version(DWORD token) {
    return 0xFFFF0000 == (token & 0xFFFF0000);
}

static inline BOOL shader_is_vshader_version(DWORD token) {
    return 0xFFFE0000 == (token & 0xFFFF0000);
}

static inline BOOL shader_is_comment(DWORD token) {
    return WINED3DSIO_COMMENT == (token & WINED3DSI_OPCODE_MASK);
}

/* TODO: vFace (ps_3_0) */
static inline BOOL shader_is_scalar(DWORD param) {
    DWORD reg_type = shader_get_regtype(param);

    switch (reg_type) {
        case WINED3DSPR_RASTOUT:
            if ((param & WINED3DSP_REGNUM_MASK) != 0) {
                /* oFog & oPts */
                return TRUE;
            }
            /* oPos */
            return FALSE;

        case WINED3DSPR_DEPTHOUT:   /* oDepth */
        case WINED3DSPR_CONSTBOOL:  /* b# */
        case WINED3DSPR_LOOP:       /* aL */
        case WINED3DSPR_PREDICATE:  /* p0 */
            return TRUE;

        default:
            return FALSE;
    }
}

/* Internally used shader constants. Applications can use constants 0 to GL_LIMITS(vshader_constantsF) - 1,
 * so upload them above that
 */
#define ARB_SHADER_PRIVCONST_BASE GL_LIMITS(vshader_constantsF)
#define ARB_SHADER_PRIVCONST_POS ARB_SHADER_PRIVCONST_BASE + 0

/*****************************************************************************
 * IDirect3DVertexShader implementation structure
 */
typedef struct IWineD3DVertexShaderImpl {
    /* IUnknown parts*/   
    const IWineD3DVertexShaderVtbl *lpVtbl;

    /* IWineD3DBaseShader */
    IWineD3DBaseShaderClass     baseShader;

    /* IWineD3DVertexShaderImpl */
    IUnknown                    *parent;

    DWORD                       usage;

    /* Vertex shader input and output semantics */
    semantic semantics_in [MAX_ATTRIBS];
    semantic semantics_out [MAX_REG_OUTPUT];

    /* Ordered array of attributes that are swizzled */
    attrib_declaration          swizzled_attribs [MAX_ATTRIBS];
    UINT                        num_swizzled_attribs;

    /* run time datas...  */
    VSHADERDATA                *data;
    UINT                       min_rel_offset, max_rel_offset;
    UINT                       rel_offset;

    UINT                       recompile_count;
#if 0 /* needs reworking */
    /* run time datas */
    VSHADERINPUTDATA input;
    VSHADEROUTPUTDATA output;
#endif
} IWineD3DVertexShaderImpl;
extern const SHADER_OPCODE IWineD3DVertexShaderImpl_shader_ins[];
extern const IWineD3DVertexShaderVtbl IWineD3DVertexShader_Vtbl;

/*****************************************************************************
 * IDirect3DPixelShader implementation structure
 */

enum vertexprocessing_mode {
    fixedfunction,
    vertexshader,
    pretransformed
};

typedef struct IWineD3DPixelShaderImpl {
    /* IUnknown parts */
    const IWineD3DPixelShaderVtbl *lpVtbl;

    /* IWineD3DBaseShader */
    IWineD3DBaseShaderClass     baseShader;

    /* IWineD3DPixelShaderImpl */
    IUnknown                   *parent;

    /* Pixel shader input semantics */
    semantic semantics_in [MAX_REG_INPUT];
    DWORD                 input_reg_map[MAX_REG_INPUT];
    BOOL                  input_reg_used[MAX_REG_INPUT];

    /* run time data */
    PSHADERDATA                *data;

    /* Some information about the shader behavior */
    char                        needsbumpmat;
    UINT                        bumpenvmatconst;
    UINT                        luminanceconst;
    char                        srgb_enabled;
    char                        srgb_mode_hardcoded;
    UINT                        srgb_low_const;
    UINT                        srgb_cmp_const;
    char                        vpos_uniform;
    BOOL                        render_offscreen;
    UINT                        height;
    enum vertexprocessing_mode  vertexprocessing;

#if 0 /* needs reworking */
    PSHADERINPUTDATA input;
    PSHADEROUTPUTDATA output;
#endif
} IWineD3DPixelShaderImpl;

extern const SHADER_OPCODE IWineD3DPixelShaderImpl_shader_ins[];
extern const IWineD3DPixelShaderVtbl IWineD3DPixelShader_Vtbl;

/* sRGB correction constants */
static const float srgb_cmp = 0.0031308;
static const float srgb_mul_low = 12.92;
static const float srgb_pow = 0.41666;
static const float srgb_mul_high = 1.055;
static const float srgb_sub_high = 0.055;

/*****************************************************************************
 * IWineD3DPalette implementation structure
 */
struct IWineD3DPaletteImpl {
    /* IUnknown parts */
    const IWineD3DPaletteVtbl  *lpVtbl;
    LONG                       ref;

    IUnknown                   *parent;
    IWineD3DDeviceImpl         *wineD3DDevice;

    /* IWineD3DPalette */
    HPALETTE                   hpal;
    WORD                       palVersion;     /*|               */
    WORD                       palNumEntries;  /*|  LOGPALETTE   */
    PALETTEENTRY               palents[256];   /*|               */
    /* This is to store the palette in 'screen format' */
    int                        screen_palents[256];
    DWORD                      Flags;
};

extern const IWineD3DPaletteVtbl IWineD3DPalette_Vtbl;
DWORD IWineD3DPaletteImpl_Size(DWORD dwFlags);

/* DirectDraw utility functions */
extern WINED3DFORMAT pixelformat_for_depth(DWORD depth);

/*****************************************************************************
 * Pixel format management
 */
typedef struct {
    WINED3DFORMAT           format;
    DWORD                   alphaMask, redMask, greenMask, blueMask;
    UINT                    bpp;
    short                   depthSize, stencilSize;
    BOOL                    isFourcc;
} StaticPixelFormatDesc;

const StaticPixelFormatDesc *getFormatDescEntry(WINED3DFORMAT fmt,
        WineD3D_GL_Info *gl_info,
        const GlPixelFormatDesc **glDesc);

static inline BOOL use_vs(IWineD3DDeviceImpl *device) {
    return (device->vs_selected_mode != SHADER_NONE
            && device->stateBlock->vertexShader
            && ((IWineD3DVertexShaderImpl *)device->stateBlock->vertexShader)->baseShader.function
            && !device->strided_streams.u.s.position_transformed);
}

static inline BOOL use_ps(IWineD3DDeviceImpl *device) {
    return (device->ps_selected_mode != SHADER_NONE
            && device->stateBlock->pixelShader
            && ((IWineD3DPixelShaderImpl *)device->stateBlock->pixelShader)->baseShader.function);
}

void stretch_rect_fbo(IWineD3DDevice *iface, IWineD3DSurface *src_surface, WINED3DRECT *src_rect,
        IWineD3DSurface *dst_surface, WINED3DRECT *dst_rect, const WINED3DTEXTUREFILTERTYPE filter, BOOL flip);

#endif
