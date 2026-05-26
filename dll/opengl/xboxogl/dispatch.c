/*
 * PROJECT:     Xbox NV2A OpenGL 1.x ICD
 * LICENSE:     GPL-2.0-or-later
 * PURPOSE:     Auto-generated stubs + dispatch table
 *
 * Every entry in the GL 1.1 dispatch table is materialised as a no-op stub
 * with the correct ABI (so __stdcall arg cleanup matches), then the table
 * is constructed from the same X-macro list opengl32 ships in glfuncs.h.
 *
 * XglDispatchInstallReal() patches the small subset of entries we actually
 * implement in real.c.  It is called once from DrvSetContext.
 */

#include "xboxogl.h"

/* Bring in GLAPIENTRY etc. */
#ifndef GLAPIENTRY
#define GLAPIENTRY APIENTRY
#endif

/* ----- 1. Stub function definitions (one per dispatch slot) -------------- */

#define USE_GL_FUNC(name, proto_args, call_args, offset, stack) \
    static void GLAPIENTRY xgl_stub_##name proto_args { (void)0; }
#define USE_GL_FUNC_RET(name, ret_type, proto_args, call_args, offset, stack) \
    static ret_type GLAPIENTRY xgl_stub_##name proto_args { ret_type _r; \
        memset(&_r, 0, sizeof(_r)); return _r; }
#include "../opengl32/glfuncs.h"
#undef USE_GL_FUNC
#undef USE_GL_FUNC_RET

/* ----- 2. Dispatch table built from those stubs -------------------------- */

GLCLTPROCTABLE g_xglDispatchTable =
{
    OPENGL_VERSION_110_ENTRIES,
    {
#define USE_GL_FUNC(name, proto_args, call_args, offset, stack) xgl_stub_##name,
#define USE_GL_FUNC_RET(name, ret_type, proto_args, call_args, offset, stack) \
        (void*)xgl_stub_##name,
#include "../opengl32/glfuncs.h"
#undef USE_GL_FUNC
#undef USE_GL_FUNC_RET
    }
};

/* ----- 3. Overlay our real implementations ------------------------------- */

void XglDispatchInstallReal(void)
{
    static int installed = 0;
    if (installed) return;
    installed = 1;

#define OVERRIDE(field, fn) g_xglDispatchTable.glDispatchTable.field = (void*)(fn)

    /* Clear */
    OVERRIDE(Clear,        xgl_real_Clear);
    OVERRIDE(ClearColor,   xgl_real_ClearColor);
    OVERRIDE(ClearDepth,   xgl_real_ClearDepth);

    /* Current attributes */
    OVERRIDE(Color4f,      xgl_real_Color4f);
    OVERRIDE(Color3f,      xgl_real_Color3f);
    OVERRIDE(Color4ub,     xgl_real_Color4ub);
    OVERRIDE(Color3ub,     xgl_real_Color3ub);
    OVERRIDE(Color4fv,     xgl_real_Color4fv);
    OVERRIDE(Color3fv,     xgl_real_Color3fv);
    OVERRIDE(Color3ubv,    xgl_real_Color3ubv);
    OVERRIDE(Color4ubv,    xgl_real_Color4ubv);
    OVERRIDE(Color3d,      xgl_real_Color3d);
    OVERRIDE(Normal3f,     xgl_real_Normal3f);
    OVERRIDE(Normal3fv,    xgl_real_Normal3fv);
    OVERRIDE(TexCoord2f,   xgl_real_TexCoord2f);
    OVERRIDE(TexCoord2fv,  xgl_real_TexCoord2fv);
    OVERRIDE(TexCoord2i,   xgl_real_TexCoord2i);

    /* Immediate mode */
    OVERRIDE(Begin,        xgl_real_Begin);
    OVERRIDE(End,          xgl_real_End);
    OVERRIDE(Vertex4f,     xgl_real_Vertex4f);
    OVERRIDE(Vertex3f,     xgl_real_Vertex3f);
    OVERRIDE(Vertex2f,     xgl_real_Vertex2f);
    OVERRIDE(Vertex2i,     xgl_real_Vertex2i);
    OVERRIDE(Vertex3i,     xgl_real_Vertex3i);
    OVERRIDE(Vertex3fv,    xgl_real_Vertex3fv);
    OVERRIDE(Vertex2fv,    xgl_real_Vertex2fv);

    /* Vertex arrays */
    OVERRIDE(VertexPointer,      xgl_real_VertexPointer);
    OVERRIDE(ColorPointer,       xgl_real_ColorPointer);
    OVERRIDE(NormalPointer,      xgl_real_NormalPointer);
    OVERRIDE(TexCoordPointer,    xgl_real_TexCoordPointer);
    OVERRIDE(EnableClientState,  xgl_real_EnableClientState);
    OVERRIDE(DisableClientState, xgl_real_DisableClientState);
    OVERRIDE(DrawArrays,         xgl_real_DrawArrays);
    OVERRIDE(DrawElements,       xgl_real_DrawElements);

    /* Matrix stack */
    OVERRIDE(MatrixMode,   xgl_real_MatrixMode);
    OVERRIDE(LoadIdentity, xgl_real_LoadIdentity);
    OVERRIDE(LoadMatrixf,  xgl_real_LoadMatrixf);
    OVERRIDE(MultMatrixf,  xgl_real_MultMatrixf);
    OVERRIDE(LoadMatrixd,  xgl_real_LoadMatrixd);
    OVERRIDE(MultMatrixd,  xgl_real_MultMatrixd);
    OVERRIDE(PushMatrix,   xgl_real_PushMatrix);
    OVERRIDE(PopMatrix,    xgl_real_PopMatrix);
    OVERRIDE(Ortho,        xgl_real_Ortho);
    OVERRIDE(Frustum,      xgl_real_Frustum);
    OVERRIDE(Translatef,   xgl_real_Translatef);
    OVERRIDE(Scalef,       xgl_real_Scalef);
    OVERRIDE(Rotatef,      xgl_real_Rotatef);
    OVERRIDE(Translated,   xgl_real_Translated);
    OVERRIDE(Scaled,       xgl_real_Scaled);
    OVERRIDE(Rotated,      xgl_real_Rotated);

    /* Viewport / depth range / scissor */
    OVERRIDE(Viewport,     xgl_real_Viewport);
    OVERRIDE(DepthRange,   xgl_real_DepthRange);
    OVERRIDE(Scissor,      xgl_real_Scissor);

    /* Render state */
    OVERRIDE(Enable,       xgl_real_Enable);
    OVERRIDE(Disable,      xgl_real_Disable);
    OVERRIDE(IsEnabled,    xgl_real_IsEnabled);
    OVERRIDE(DepthFunc,    xgl_real_DepthFunc);
    OVERRIDE(DepthMask,    xgl_real_DepthMask);
    OVERRIDE(ShadeModel,   xgl_real_ShadeModel);
    OVERRIDE(CullFace,     xgl_real_CullFace);
    OVERRIDE(FrontFace,    xgl_real_FrontFace);
    OVERRIDE(BlendFunc,    xgl_real_BlendFunc);
    OVERRIDE(Hint,         xgl_real_Hint);

    /* Textures */
    OVERRIDE(GenTextures,    xgl_real_GenTextures);
    OVERRIDE(DeleteTextures, xgl_real_DeleteTextures);
    OVERRIDE(BindTexture,    xgl_real_BindTexture);
    OVERRIDE(TexImage2D,     xgl_real_TexImage2D);
    OVERRIDE(TexSubImage2D,  xgl_real_TexSubImage2D);
    OVERRIDE(TexParameteri,  xgl_real_TexParameteri);
    OVERRIDE(TexParameterf,  xgl_real_TexParameterf);
    OVERRIDE(TexEnvi,        xgl_real_TexEnvi);
    OVERRIDE(TexEnvf,        xgl_real_TexEnvf);

    /* Queries */
    OVERRIDE(GetError,     xgl_real_GetError);
    OVERRIDE(GetIntegerv,  xgl_real_GetIntegerv);
    OVERRIDE(GetFloatv,    xgl_real_GetFloatv);
    OVERRIDE(GetString,    xgl_real_GetString);

    OVERRIDE(Finish,       xgl_real_Finish);
    OVERRIDE(Flush,        xgl_real_Flush);

    /* Display lists */
    OVERRIDE(GenLists,     xgl_real_GenLists);
    OVERRIDE(NewList,      xgl_real_NewList);
    OVERRIDE(EndList,      xgl_real_EndList);
    OVERRIDE(CallList,     xgl_real_CallList);
    OVERRIDE(DeleteLists,  xgl_real_DeleteLists);
    OVERRIDE(IsList,       xgl_real_IsList);

    /* Lighting / materials (mapped to current colour; no HW lighting) */
    OVERRIDE(Materialfv,   xgl_real_Materialfv);
    OVERRIDE(Materialf,    xgl_real_Materialf);
    OVERRIDE(Lightfv,      xgl_real_Lightfv);
    OVERRIDE(Lightf,       xgl_real_Lightf);
    OVERRIDE(LightModelfv, xgl_real_LightModelfv);
    OVERRIDE(ColorMaterial, xgl_real_ColorMaterial);
    OVERRIDE(Fogf,         xgl_real_Fogf);
    OVERRIDE(Fogi,         xgl_real_Fogi);
    OVERRIDE(Fogfv,        xgl_real_Fogfv);
    OVERRIDE(AlphaFunc,    xgl_real_AlphaFunc);
    OVERRIDE(PolygonMode,  xgl_real_PolygonMode);
    OVERRIDE(PolygonOffset, xgl_real_PolygonOffset);

    /* Type-variant entry points */
    OVERRIDE(Vertex2d,     xgl_real_Vertex2d);
    OVERRIDE(Vertex3d,     xgl_real_Vertex3d);
    OVERRIDE(Vertex2s,     xgl_real_Vertex2s);
    OVERRIDE(Vertex3s,     xgl_real_Vertex3s);
    OVERRIDE(Vertex4i,     xgl_real_Vertex4i);
    OVERRIDE(Vertex2dv,    xgl_real_Vertex2dv);
    OVERRIDE(Vertex3dv,    xgl_real_Vertex3dv);
    OVERRIDE(Vertex4fv,    xgl_real_Vertex4fv);
    OVERRIDE(Color4d,      xgl_real_Color4d);
    OVERRIDE(Color3b,      xgl_real_Color3b);
    OVERRIDE(Color4b,      xgl_real_Color4b);
    OVERRIDE(Color4dv,     xgl_real_Color4dv);
    OVERRIDE(Color3dv,     xgl_real_Color3dv);
    OVERRIDE(Normal3d,     xgl_real_Normal3d);
    OVERRIDE(Normal3b,     xgl_real_Normal3b);
    OVERRIDE(Normal3dv,    xgl_real_Normal3dv);
    OVERRIDE(TexCoord1f,   xgl_real_TexCoord1f);
    OVERRIDE(TexCoord3f,   xgl_real_TexCoord3f);
    OVERRIDE(TexCoord4f,   xgl_real_TexCoord4f);
    OVERRIDE(TexCoord2d,   xgl_real_TexCoord2d);
    OVERRIDE(TexCoord1d,   xgl_real_TexCoord1d);
    OVERRIDE(TexCoord3fv,  xgl_real_TexCoord3fv);
    OVERRIDE(TexCoord4fv,  xgl_real_TexCoord4fv);
    OVERRIDE(Rectf,        xgl_real_Rectf);
    OVERRIDE(Recti,        xgl_real_Recti);
    OVERRIDE(Rects,        xgl_real_Rects);
    OVERRIDE(Rectd,        xgl_real_Rectd);
    OVERRIDE(PointSize,    xgl_real_PointSize);
    OVERRIDE(LineWidth,    xgl_real_LineWidth);
    OVERRIDE(PushAttrib,   xgl_real_PushAttrib);
    OVERRIDE(PopAttrib,    xgl_real_PopAttrib);
    /* ---- Completed GL 1.1 feature set ---- */
    /* Display lists */
    OVERRIDE(CallLists,    xgl_real_CallLists);
    OVERRIDE(ListBase,     xgl_real_ListBase);

    /* Colour / normal / vertex / texcoord / rect type variants */
    OVERRIDE(Color3bv,  xgl_real_Color3bv);
    OVERRIDE(Color3i,   xgl_real_Color3i);
    OVERRIDE(Color3iv,  xgl_real_Color3iv);
    OVERRIDE(Color3s,   xgl_real_Color3s);
    OVERRIDE(Color3sv,  xgl_real_Color3sv);
    OVERRIDE(Color3ui,  xgl_real_Color3ui);
    OVERRIDE(Color3uiv, xgl_real_Color3uiv);
    OVERRIDE(Color3us,  xgl_real_Color3us);
    OVERRIDE(Color3usv, xgl_real_Color3usv);
    OVERRIDE(Color4bv,  xgl_real_Color4bv);
    OVERRIDE(Color4i,   xgl_real_Color4i);
    OVERRIDE(Color4iv,  xgl_real_Color4iv);
    OVERRIDE(Color4s,   xgl_real_Color4s);
    OVERRIDE(Color4sv,  xgl_real_Color4sv);
    OVERRIDE(Color4ui,  xgl_real_Color4ui);
    OVERRIDE(Color4uiv, xgl_real_Color4uiv);
    OVERRIDE(Color4us,  xgl_real_Color4us);
    OVERRIDE(Color4usv, xgl_real_Color4usv);
    OVERRIDE(Normal3bv, xgl_real_Normal3bv);
    OVERRIDE(Normal3i,  xgl_real_Normal3i);
    OVERRIDE(Normal3iv, xgl_real_Normal3iv);
    OVERRIDE(Normal3s,  xgl_real_Normal3s);
    OVERRIDE(Normal3sv, xgl_real_Normal3sv);
    OVERRIDE(Vertex2iv, xgl_real_Vertex2iv);
    OVERRIDE(Vertex2sv, xgl_real_Vertex2sv);
    OVERRIDE(Vertex3iv, xgl_real_Vertex3iv);
    OVERRIDE(Vertex3sv, xgl_real_Vertex3sv);
    OVERRIDE(Vertex4d,  xgl_real_Vertex4d);
    OVERRIDE(Vertex4dv, xgl_real_Vertex4dv);
    OVERRIDE(Vertex4iv, xgl_real_Vertex4iv);
    OVERRIDE(Vertex4s,  xgl_real_Vertex4s);
    OVERRIDE(Vertex4sv, xgl_real_Vertex4sv);
    OVERRIDE(TexCoord1fv, xgl_real_TexCoord1fv);
    OVERRIDE(TexCoord1i,  xgl_real_TexCoord1i);
    OVERRIDE(TexCoord1iv, xgl_real_TexCoord1iv);
    OVERRIDE(TexCoord1s,  xgl_real_TexCoord1s);
    OVERRIDE(TexCoord1sv, xgl_real_TexCoord1sv);
    OVERRIDE(TexCoord1dv, xgl_real_TexCoord1dv);
    OVERRIDE(TexCoord2dv, xgl_real_TexCoord2dv);
    OVERRIDE(TexCoord2iv, xgl_real_TexCoord2iv);
    OVERRIDE(TexCoord2s,  xgl_real_TexCoord2s);
    OVERRIDE(TexCoord2sv, xgl_real_TexCoord2sv);
    OVERRIDE(TexCoord3d,  xgl_real_TexCoord3d);
    OVERRIDE(TexCoord3dv, xgl_real_TexCoord3dv);
    OVERRIDE(TexCoord3i,  xgl_real_TexCoord3i);
    OVERRIDE(TexCoord3iv, xgl_real_TexCoord3iv);
    OVERRIDE(TexCoord3s,  xgl_real_TexCoord3s);
    OVERRIDE(TexCoord3sv, xgl_real_TexCoord3sv);
    OVERRIDE(TexCoord4d,  xgl_real_TexCoord4d);
    OVERRIDE(TexCoord4dv, xgl_real_TexCoord4dv);
    OVERRIDE(TexCoord4i,  xgl_real_TexCoord4i);
    OVERRIDE(TexCoord4iv, xgl_real_TexCoord4iv);
    OVERRIDE(TexCoord4s,  xgl_real_TexCoord4s);
    OVERRIDE(TexCoord4sv, xgl_real_TexCoord4sv);
    OVERRIDE(Rectdv,    xgl_real_Rectdv);
    OVERRIDE(Rectfv,    xgl_real_Rectfv);
    OVERRIDE(Rectiv,    xgl_real_Rectiv);
    OVERRIDE(Rectsv,    xgl_real_Rectsv);
    OVERRIDE(EdgeFlag,  xgl_real_EdgeFlag);
    OVERRIDE(EdgeFlagv, xgl_real_EdgeFlagv);

    /* Colour-index (tracked) */
    OVERRIDE(Indexd,  xgl_real_Indexd);
    OVERRIDE(Indexdv, xgl_real_Indexdv);
    OVERRIDE(Indexf,  xgl_real_Indexf);
    OVERRIDE(Indexfv, xgl_real_Indexfv);
    OVERRIDE(Indexi,  xgl_real_Indexi);
    OVERRIDE(Indexiv, xgl_real_Indexiv);
    OVERRIDE(Indexs,  xgl_real_Indexs);
    OVERRIDE(Indexsv, xgl_real_Indexsv);
    OVERRIDE(Indexub, xgl_real_Indexub);
    OVERRIDE(Indexubv,xgl_real_Indexubv);
    OVERRIDE(IndexMask, xgl_real_IndexMask);
    OVERRIDE(ClearIndex,xgl_real_ClearIndex);

    /* Integer / array variants of state setters */
    OVERRIDE(Fogiv,        xgl_real_Fogiv);
    OVERRIDE(Lighti,       xgl_real_Lighti);
    OVERRIDE(Lightiv,      xgl_real_Lightiv);
    OVERRIDE(LightModelf,  xgl_real_LightModelf);
    OVERRIDE(LightModeli,  xgl_real_LightModeli);
    OVERRIDE(LightModeliv, xgl_real_LightModeliv);
    OVERRIDE(Materiali,    xgl_real_Materiali);
    OVERRIDE(Materialiv,   xgl_real_Materialiv);
    OVERRIDE(TexParameterfv, xgl_real_TexParameterfv);
    OVERRIDE(TexParameteriv, xgl_real_TexParameteriv);
    OVERRIDE(TexEnvfv,     xgl_real_TexEnvfv);
    OVERRIDE(TexEnviv,     xgl_real_TexEnviv);

    /* Texture coordinate generation */
    OVERRIDE(TexGeni,    xgl_real_TexGeni);
    OVERRIDE(TexGenf,    xgl_real_TexGenf);
    OVERRIDE(TexGend,    xgl_real_TexGend);
    OVERRIDE(TexGeniv,   xgl_real_TexGeniv);
    OVERRIDE(TexGenfv,   xgl_real_TexGenfv);
    OVERRIDE(TexGendv,   xgl_real_TexGendv);
    OVERRIDE(GetTexGeniv, xgl_real_GetTexGeniv);
    OVERRIDE(GetTexGenfv, xgl_real_GetTexGenfv);
    OVERRIDE(GetTexGendv, xgl_real_GetTexGendv);

    /* 1D textures */
    OVERRIDE(TexImage1D,    xgl_real_TexImage1D);
    OVERRIDE(TexSubImage1D, xgl_real_TexSubImage1D);

    /* Clip planes */
    OVERRIDE(ClipPlane,    xgl_real_ClipPlane);
    OVERRIDE(GetClipPlane, xgl_real_GetClipPlane);

    /* Framebuffer masks / misc render state */
    OVERRIDE(ColorMask,    xgl_real_ColorMask);
    OVERRIDE(LineStipple,  xgl_real_LineStipple);
    OVERRIDE(PolygonStipple, xgl_real_PolygonStipple);
    OVERRIDE(GetPolygonStipple, xgl_real_GetPolygonStipple);
    OVERRIDE(LogicOp,      xgl_real_LogicOp);
    OVERRIDE(DrawBuffer,   xgl_real_DrawBuffer);
    OVERRIDE(ReadBuffer,   xgl_real_ReadBuffer);
    OVERRIDE(ClearAccum,   xgl_real_ClearAccum);
    OVERRIDE(Accum,        xgl_real_Accum);
    OVERRIDE(ClearStencil, xgl_real_ClearStencil);
    OVERRIDE(StencilMask,  xgl_real_StencilMask);
    OVERRIDE(StencilFunc,  xgl_real_StencilFunc);
    OVERRIDE(StencilOp,    xgl_real_StencilOp);

    /* Raster position / pixel ops */
    OVERRIDE(RasterPos2f,  xgl_real_RasterPos2f);
    OVERRIDE(RasterPos2i,  xgl_real_RasterPos2i);
    OVERRIDE(RasterPos2d,  xgl_real_RasterPos2d);
    OVERRIDE(RasterPos2s,  xgl_real_RasterPos2s);
    OVERRIDE(RasterPos3f,  xgl_real_RasterPos3f);
    OVERRIDE(RasterPos3i,  xgl_real_RasterPos3i);
    OVERRIDE(RasterPos3d,  xgl_real_RasterPos3d);
    OVERRIDE(RasterPos3s,  xgl_real_RasterPos3s);
    OVERRIDE(RasterPos4f,  xgl_real_RasterPos4f);
    OVERRIDE(RasterPos4i,  xgl_real_RasterPos4i);
    OVERRIDE(RasterPos4d,  xgl_real_RasterPos4d);
    OVERRIDE(RasterPos4s,  xgl_real_RasterPos4s);
    OVERRIDE(RasterPos2fv, xgl_real_RasterPos2fv);
    OVERRIDE(RasterPos2iv, xgl_real_RasterPos2iv);
    OVERRIDE(RasterPos2dv, xgl_real_RasterPos2dv);
    OVERRIDE(RasterPos2sv, xgl_real_RasterPos2sv);
    OVERRIDE(RasterPos3fv, xgl_real_RasterPos3fv);
    OVERRIDE(RasterPos3iv, xgl_real_RasterPos3iv);
    OVERRIDE(RasterPos3dv, xgl_real_RasterPos3dv);
    OVERRIDE(RasterPos3sv, xgl_real_RasterPos3sv);
    OVERRIDE(RasterPos4fv, xgl_real_RasterPos4fv);
    OVERRIDE(RasterPos4iv, xgl_real_RasterPos4iv);
    OVERRIDE(RasterPos4dv, xgl_real_RasterPos4dv);
    OVERRIDE(RasterPos4sv, xgl_real_RasterPos4sv);
    OVERRIDE(DrawPixels,   xgl_real_DrawPixels);
    OVERRIDE(Bitmap,       xgl_real_Bitmap);
    OVERRIDE(PixelZoom,    xgl_real_PixelZoom);
    OVERRIDE(PixelStorei,  xgl_real_PixelStorei);
    OVERRIDE(PixelStoref,  xgl_real_PixelStoref);
    OVERRIDE(PixelTransferi, xgl_real_PixelTransferi);
    OVERRIDE(PixelTransferf, xgl_real_PixelTransferf);
    OVERRIDE(PixelMapfv,   xgl_real_PixelMapfv);
    OVERRIDE(PixelMapuiv,  xgl_real_PixelMapuiv);
    OVERRIDE(PixelMapusv,  xgl_real_PixelMapusv);
    OVERRIDE(GetPixelMapfv, xgl_real_GetPixelMapfv);
    OVERRIDE(GetPixelMapuiv, xgl_real_GetPixelMapuiv);
    OVERRIDE(GetPixelMapusv, xgl_real_GetPixelMapusv);
    OVERRIDE(ReadPixels,   xgl_real_ReadPixels);
    OVERRIDE(CopyPixels,   xgl_real_CopyPixels);
    OVERRIDE(CopyTexImage1D,    xgl_real_CopyTexImage1D);
    OVERRIDE(CopyTexSubImage1D, xgl_real_CopyTexSubImage1D);
    OVERRIDE(CopyTexImage2D,    xgl_real_CopyTexImage2D);
    OVERRIDE(CopyTexSubImage2D, xgl_real_CopyTexSubImage2D);

    /* Vertex-array completeness */
    OVERRIDE(ArrayElement, xgl_real_ArrayElement);
    OVERRIDE(InterleavedArrays, xgl_real_InterleavedArrays);
    OVERRIDE(IndexPointer, xgl_real_IndexPointer);
    OVERRIDE(EdgeFlagPointer, xgl_real_EdgeFlagPointer);
    OVERRIDE(PushClientAttrib, xgl_real_PushClientAttrib);
    OVERRIDE(PopClientAttrib,  xgl_real_PopClientAttrib);
    OVERRIDE(GetPointerv,  xgl_real_GetPointerv);

    /* Texture object queries */
    OVERRIDE(IsTexture,    xgl_real_IsTexture);
    OVERRIDE(AreTexturesResident, xgl_real_AreTexturesResident);
    OVERRIDE(PrioritizeTextures,  xgl_real_PrioritizeTextures);

    /* Complete glGet* family */
    OVERRIDE(GetBooleanv,  xgl_real_GetBooleanv);
    OVERRIDE(GetDoublev,   xgl_real_GetDoublev);
    OVERRIDE(GetLightfv,   xgl_real_GetLightfv);
    OVERRIDE(GetLightiv,   xgl_real_GetLightiv);
    OVERRIDE(GetMaterialfv, xgl_real_GetMaterialfv);
    OVERRIDE(GetMaterialiv, xgl_real_GetMaterialiv);
    OVERRIDE(GetTexEnvfv,  xgl_real_GetTexEnvfv);
    OVERRIDE(GetTexEnviv,  xgl_real_GetTexEnviv);
    OVERRIDE(GetTexParameterfv, xgl_real_GetTexParameterfv);
    OVERRIDE(GetTexParameteriv, xgl_real_GetTexParameteriv);
    OVERRIDE(GetTexLevelParameterfv, xgl_real_GetTexLevelParameterfv);
    OVERRIDE(GetTexLevelParameteriv, xgl_real_GetTexLevelParameteriv);
    OVERRIDE(GetTexImage,  xgl_real_GetTexImage);
    OVERRIDE(GetMapfv,     xgl_real_GetMapfv);
    OVERRIDE(GetMapdv,     xgl_real_GetMapdv);
    OVERRIDE(GetMapiv,     xgl_real_GetMapiv);

    /* Selection + feedback */
    OVERRIDE(SelectBuffer,   xgl_real_SelectBuffer);
    OVERRIDE(FeedbackBuffer, xgl_real_FeedbackBuffer);
    OVERRIDE(RenderMode,     xgl_real_RenderMode);
    OVERRIDE(InitNames,      xgl_real_InitNames);
    OVERRIDE(LoadName,       xgl_real_LoadName);
    OVERRIDE(PushName,       xgl_real_PushName);
    OVERRIDE(PopName,        xgl_real_PopName);
    OVERRIDE(PassThrough,    xgl_real_PassThrough);

    /* Evaluators */
    OVERRIDE(Map1f,        xgl_real_Map1f);
    OVERRIDE(Map1d,        xgl_real_Map1d);
    OVERRIDE(Map2f,        xgl_real_Map2f);
    OVERRIDE(Map2d,        xgl_real_Map2d);
    OVERRIDE(MapGrid1f,    xgl_real_MapGrid1f);
    OVERRIDE(MapGrid1d,    xgl_real_MapGrid1d);
    OVERRIDE(MapGrid2f,    xgl_real_MapGrid2f);
    OVERRIDE(MapGrid2d,    xgl_real_MapGrid2d);
    OVERRIDE(EvalCoord1f,  xgl_real_EvalCoord1f);
    OVERRIDE(EvalCoord1d,  xgl_real_EvalCoord1d);
    OVERRIDE(EvalCoord1fv, xgl_real_EvalCoord1fv);
    OVERRIDE(EvalCoord1dv, xgl_real_EvalCoord1dv);
    OVERRIDE(EvalCoord2f,  xgl_real_EvalCoord2f);
    OVERRIDE(EvalCoord2d,  xgl_real_EvalCoord2d);
    OVERRIDE(EvalCoord2fv, xgl_real_EvalCoord2fv);
    OVERRIDE(EvalCoord2dv, xgl_real_EvalCoord2dv);
    OVERRIDE(EvalMesh1,    xgl_real_EvalMesh1);
    OVERRIDE(EvalMesh2,    xgl_real_EvalMesh2);
    OVERRIDE(EvalPoint1,   xgl_real_EvalPoint1);
    OVERRIDE(EvalPoint2,   xgl_real_EvalPoint2);

    /* These are only for the defaults (Nothing using the proc api) */
#undef OVERRIDE
}
