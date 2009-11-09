/*
 * Mesa 3-D graphics library
 * Version:  7.1
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


#include "main/glheader.h"
#include "main/imports.h"
#include "main/arrayobj.h"
#include "main/buffers.h"
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/mipmap.h"
#include "main/queryobj.h"
#include "main/renderbuffer.h"
#include "main/texcompress.h"
#include "main/texformat.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/texstore.h"
#if FEATURE_ARB_vertex_buffer_object
#include "main/bufferobj.h"
#endif
#if FEATURE_EXT_framebuffer_object
#include "main/fbobject.h"
#include "main/texrender.h"
#endif

#include "shader/program.h"
#include "shader/prog_execute.h"
#include "shader/shader_api.h"
#include "tnl/tnl.h"
#include "swrast/swrast.h"

#include "driverfuncs.h"



/**
 * Plug in default functions for all pointers in the dd_function_table
 * structure.
 * Device drivers should call this function and then plug in any
 * functions which it wants to override.
 * Some functions (pointers) MUST be implemented by all drivers (REQUIRED).
 *
 * \param table the dd_function_table to initialize
 */
void
_mesa_init_driver_functions(struct dd_function_table *driver)
{
   _mesa_bzero(driver, sizeof(*driver));

   driver->GetString = NULL;  /* REQUIRED! */
   driver->UpdateState = NULL;  /* REQUIRED! */
   driver->GetBufferSize = NULL;  /* REQUIRED! */
   driver->ResizeBuffers = _mesa_resize_framebuffer;
   driver->Error = NULL;

   driver->Finish = NULL;
   driver->Flush = NULL;

   /* framebuffer/image functions */
   driver->Clear = _swrast_Clear;
   driver->Accum = _swrast_Accum;
   driver->RasterPos = _tnl_RasterPos;
   driver->DrawPixels = _swrast_DrawPixels;
   driver->ReadPixels = _swrast_ReadPixels;
   driver->CopyPixels = _swrast_CopyPixels;
   driver->Bitmap = _swrast_Bitmap;

   /* Texture functions */
   driver->ChooseTextureFormat = _mesa_choose_tex_format;
   driver->TexImage1D = _mesa_store_teximage1d;
   driver->TexImage2D = _mesa_store_teximage2d;
   driver->TexImage3D = _mesa_store_teximage3d;
   driver->TexSubImage1D = _mesa_store_texsubimage1d;
   driver->TexSubImage2D = _mesa_store_texsubimage2d;
   driver->TexSubImage3D = _mesa_store_texsubimage3d;
   driver->GetTexImage = _mesa_get_teximage;
   driver->CopyTexImage1D = _swrast_copy_teximage1d;
   driver->CopyTexImage2D = _swrast_copy_teximage2d;
   driver->CopyTexSubImage1D = _swrast_copy_texsubimage1d;
   driver->CopyTexSubImage2D = _swrast_copy_texsubimage2d;
   driver->CopyTexSubImage3D = _swrast_copy_texsubimage3d;
   driver->GenerateMipmap = _mesa_generate_mipmap;
   driver->TestProxyTexImage = _mesa_test_proxy_teximage;
   driver->CompressedTexImage1D = _mesa_store_compressed_teximage1d;
   driver->CompressedTexImage2D = _mesa_store_compressed_teximage2d;
   driver->CompressedTexImage3D = _mesa_store_compressed_teximage3d;
   driver->CompressedTexSubImage1D = _mesa_store_compressed_texsubimage1d;
   driver->CompressedTexSubImage2D = _mesa_store_compressed_texsubimage2d;
   driver->CompressedTexSubImage3D = _mesa_store_compressed_texsubimage3d;
   driver->GetCompressedTexImage = _mesa_get_compressed_teximage;
   driver->CompressedTextureSize = _mesa_compressed_texture_size;
   driver->BindTexture = NULL;
   driver->NewTextureObject = _mesa_new_texture_object;
   driver->DeleteTexture = _mesa_delete_texture_object;
   driver->NewTextureImage = _mesa_new_texture_image;
   driver->FreeTexImageData = _mesa_free_texture_image_data; 
   driver->MapTexture = NULL;
   driver->UnmapTexture = NULL;
   driver->TextureMemCpy = _mesa_memcpy; 
   driver->IsTextureResident = NULL;
   driver->PrioritizeTexture = NULL;
   driver->ActiveTexture = NULL;
   driver->UpdateTexturePalette = NULL;

   /* imaging */
   driver->CopyColorTable = _swrast_CopyColorTable;
   driver->CopyColorSubTable = _swrast_CopyColorSubTable;
   driver->CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   driver->CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;

   /* Vertex/fragment programs */
   driver->BindProgram = NULL;
   driver->NewProgram = _mesa_new_program;
   driver->DeleteProgram = _mesa_delete_program;
#if FEATURE_MESA_program_debug
   driver->GetProgramRegister = _mesa_get_program_register;
#endif /* FEATURE_MESA_program_debug */

   /* simple state commands */
   driver->AlphaFunc = NULL;
   driver->BlendColor = NULL;
   driver->BlendEquationSeparate = NULL;
   driver->BlendFuncSeparate = NULL;
   driver->ClearColor = NULL;
   driver->ClearDepth = NULL;
   driver->ClearIndex = NULL;
   driver->ClearStencil = NULL;
   driver->ClipPlane = NULL;
   driver->ColorMask = NULL;
   driver->ColorMaterial = NULL;
   driver->CullFace = NULL;
   driver->DrawBuffer = NULL;
   driver->DrawBuffers = NULL;
   driver->FrontFace = NULL;
   driver->DepthFunc = NULL;
   driver->DepthMask = NULL;
   driver->DepthRange = NULL;
   driver->Enable = NULL;
   driver->Fogfv = NULL;
   driver->Hint = NULL;
   driver->IndexMask = NULL;
   driver->Lightfv = NULL;
   driver->LightModelfv = NULL;
   driver->LineStipple = NULL;
   driver->LineWidth = NULL;
   driver->LogicOpcode = NULL;
   driver->PointParameterfv = NULL;
   driver->PointSize = NULL;
   driver->PolygonMode = NULL;
   driver->PolygonOffset = NULL;
   driver->PolygonStipple = NULL;
   driver->ReadBuffer = NULL;
   driver->RenderMode = NULL;
   driver->Scissor = NULL;
   driver->ShadeModel = NULL;
   driver->StencilFuncSeparate = NULL;
   driver->StencilOpSeparate = NULL;
   driver->StencilMaskSeparate = NULL;
   driver->TexGen = NULL;
   driver->TexEnv = NULL;
   driver->TexParameter = NULL;
   driver->TextureMatrix = NULL;
   driver->Viewport = NULL;

   /* vertex arrays */
   driver->VertexPointer = NULL;
   driver->NormalPointer = NULL;
   driver->ColorPointer = NULL;
   driver->FogCoordPointer = NULL;
   driver->IndexPointer = NULL;
   driver->SecondaryColorPointer = NULL;
   driver->TexCoordPointer = NULL;
   driver->EdgeFlagPointer = NULL;
   driver->VertexAttribPointer = NULL;
   driver->LockArraysEXT = NULL;
   driver->UnlockArraysEXT = NULL;

   /* state queries */
   driver->GetBooleanv = NULL;
   driver->GetDoublev = NULL;
   driver->GetFloatv = NULL;
   driver->GetIntegerv = NULL;
   driver->GetPointerv = NULL;
   
#if FEATURE_ARB_vertex_buffer_object
   driver->NewBufferObject = _mesa_new_buffer_object;
   driver->DeleteBuffer = _mesa_delete_buffer_object;
   driver->BindBuffer = NULL;
   driver->BufferData = _mesa_buffer_data;
   driver->BufferSubData = _mesa_buffer_subdata;
   driver->GetBufferSubData = _mesa_buffer_get_subdata;
   driver->MapBuffer = _mesa_buffer_map;
   driver->UnmapBuffer = _mesa_buffer_unmap;
#endif

#if FEATURE_EXT_framebuffer_object
   driver->NewFramebuffer = _mesa_new_framebuffer;
   driver->NewRenderbuffer = _mesa_new_soft_renderbuffer;
   driver->RenderTexture = _mesa_render_texture;
   driver->FinishRenderTexture = _mesa_finish_render_texture;
   driver->FramebufferRenderbuffer = _mesa_framebuffer_renderbuffer;
#endif

#if FEATURE_EXT_framebuffer_blit
   driver->BlitFramebuffer = _swrast_BlitFramebuffer;
#endif

   /* query objects */
   driver->NewQueryObject = _mesa_new_query_object;
   driver->DeleteQuery = _mesa_delete_query;
   driver->BeginQuery = _mesa_begin_query;
   driver->EndQuery = _mesa_end_query;
   driver->WaitQuery = _mesa_wait_query;

   /* APPLE_vertex_array_object */
   driver->NewArrayObject = _mesa_new_array_object;
   driver->DeleteArrayObject = _mesa_delete_array_object;
   driver->BindArrayObject = NULL;

   /* T&L stuff */
   driver->NeedValidate = GL_FALSE;
   driver->ValidateTnlModule = NULL;
   driver->CurrentExecPrimitive = 0;
   driver->CurrentSavePrimitive = 0;
   driver->NeedFlush = 0;
   driver->SaveNeedFlush = 0;

   driver->ProgramStringNotify = _tnl_program_string;
   driver->FlushVertices = NULL;
   driver->SaveFlushVertices = NULL;
   driver->NotifySaveBegin = NULL;
   driver->LightingSpaceChange = NULL;

   /* display list */
   driver->NewList = NULL;
   driver->EndList = NULL;
   driver->BeginCallList = NULL;
   driver->EndCallList = NULL;


   /* XXX temporary here */
   _mesa_init_glsl_driver_functions(driver);
}


/**
 * Call the ctx->Driver.* state functions with current values to initialize
 * driver state.
 * Only the Intel drivers use this so far.
 */
void
_mesa_init_driver_state(GLcontext *ctx)
{
   ctx->Driver.AlphaFunc(ctx, ctx->Color.AlphaFunc, ctx->Color.AlphaRef);

   ctx->Driver.BlendColor(ctx, ctx->Color.BlendColor);

   ctx->Driver.BlendEquationSeparate(ctx,
                                     ctx->Color.BlendEquationRGB,
                                     ctx->Color.BlendEquationA);

   ctx->Driver.BlendFuncSeparate(ctx,
                                 ctx->Color.BlendSrcRGB,
                                 ctx->Color.BlendDstRGB,
                                 ctx->Color.BlendSrcA, ctx->Color.BlendDstA);

   ctx->Driver.ColorMask(ctx,
                         ctx->Color.ColorMask[RCOMP],
                         ctx->Color.ColorMask[GCOMP],
                         ctx->Color.ColorMask[BCOMP],
                         ctx->Color.ColorMask[ACOMP]);

   ctx->Driver.CullFace(ctx, ctx->Polygon.CullFaceMode);
   ctx->Driver.DepthFunc(ctx, ctx->Depth.Func);
   ctx->Driver.DepthMask(ctx, ctx->Depth.Mask);

   ctx->Driver.Enable(ctx, GL_ALPHA_TEST, ctx->Color.AlphaEnabled);
   ctx->Driver.Enable(ctx, GL_BLEND, ctx->Color.BlendEnabled);
   ctx->Driver.Enable(ctx, GL_COLOR_LOGIC_OP, ctx->Color.ColorLogicOpEnabled);
   ctx->Driver.Enable(ctx, GL_COLOR_SUM, ctx->Fog.ColorSumEnabled);
   ctx->Driver.Enable(ctx, GL_CULL_FACE, ctx->Polygon.CullFlag);
   ctx->Driver.Enable(ctx, GL_DEPTH_TEST, ctx->Depth.Test);
   ctx->Driver.Enable(ctx, GL_DITHER, ctx->Color.DitherFlag);
   ctx->Driver.Enable(ctx, GL_FOG, ctx->Fog.Enabled);
   ctx->Driver.Enable(ctx, GL_LIGHTING, ctx->Light.Enabled);
   ctx->Driver.Enable(ctx, GL_LINE_SMOOTH, ctx->Line.SmoothFlag);
   ctx->Driver.Enable(ctx, GL_POLYGON_STIPPLE, ctx->Polygon.StippleFlag);
   ctx->Driver.Enable(ctx, GL_SCISSOR_TEST, ctx->Scissor.Enabled);
   ctx->Driver.Enable(ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled);
   ctx->Driver.Enable(ctx, GL_TEXTURE_1D, GL_FALSE);
   ctx->Driver.Enable(ctx, GL_TEXTURE_2D, GL_FALSE);
   ctx->Driver.Enable(ctx, GL_TEXTURE_RECTANGLE_NV, GL_FALSE);
   ctx->Driver.Enable(ctx, GL_TEXTURE_3D, GL_FALSE);
   ctx->Driver.Enable(ctx, GL_TEXTURE_CUBE_MAP, GL_FALSE);

   ctx->Driver.Fogfv(ctx, GL_FOG_COLOR, ctx->Fog.Color);
   ctx->Driver.Fogfv(ctx, GL_FOG_MODE, 0);
   ctx->Driver.Fogfv(ctx, GL_FOG_DENSITY, &ctx->Fog.Density);
   ctx->Driver.Fogfv(ctx, GL_FOG_START, &ctx->Fog.Start);
   ctx->Driver.Fogfv(ctx, GL_FOG_END, &ctx->Fog.End);

   ctx->Driver.FrontFace(ctx, ctx->Polygon.FrontFace);

   {
      GLfloat f = (GLfloat) ctx->Light.Model.ColorControl;
      ctx->Driver.LightModelfv(ctx, GL_LIGHT_MODEL_COLOR_CONTROL, &f);
   }

   ctx->Driver.LineWidth(ctx, ctx->Line.Width);
   ctx->Driver.LogicOpcode(ctx, ctx->Color.LogicOp);
   ctx->Driver.PointSize(ctx, ctx->Point.Size);
   ctx->Driver.PolygonStipple(ctx, (const GLubyte *) ctx->PolygonStipple);
   ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
                       ctx->Scissor.Width, ctx->Scissor.Height);
   ctx->Driver.ShadeModel(ctx, ctx->Light.ShadeModel);
   ctx->Driver.StencilFuncSeparate(ctx, GL_FRONT,
                                   ctx->Stencil.Function[0],
                                   ctx->Stencil.Ref[0],
                                   ctx->Stencil.ValueMask[0]);
   ctx->Driver.StencilFuncSeparate(ctx, GL_BACK,
                                   ctx->Stencil.Function[1],
                                   ctx->Stencil.Ref[1],
                                   ctx->Stencil.ValueMask[1]);
   ctx->Driver.StencilMaskSeparate(ctx, GL_FRONT, ctx->Stencil.WriteMask[0]);
   ctx->Driver.StencilMaskSeparate(ctx, GL_BACK, ctx->Stencil.WriteMask[1]);
   ctx->Driver.StencilOpSeparate(ctx, GL_FRONT,
                                 ctx->Stencil.FailFunc[0],
                                 ctx->Stencil.ZFailFunc[0],
                                 ctx->Stencil.ZPassFunc[0]);
   ctx->Driver.StencilOpSeparate(ctx, GL_BACK,
                                 ctx->Stencil.FailFunc[1],
                                 ctx->Stencil.ZFailFunc[1],
                                 ctx->Stencil.ZPassFunc[1]);


   ctx->Driver.DrawBuffer(ctx, ctx->Color.DrawBuffer[0]);
}
