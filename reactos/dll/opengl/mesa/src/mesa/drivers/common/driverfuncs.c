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
#include "main/accum.h"
#include "main/arrayobj.h"
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/mipmap.h"
#include "main/readpix.h"
#include "main/renderbuffer.h"
#include "main/shaderobj.h"
#include "main/texformat.h"
#include "main/texgetimage.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/texstore.h"
#include "main/bufferobj.h"
#include "main/fbobject.h"
#include "main/texturebarrier.h"

#include "program/program.h"
#include "tnl/tnl.h"
#include "swrast/swrast.h"
#include "swrast/s_renderbuffer.h"

#include "driverfuncs.h"
#include "meta.h"



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
   memset(driver, 0, sizeof(*driver));

   driver->GetString = NULL;  /* REQUIRED! */
   driver->UpdateState = NULL;  /* REQUIRED! */
   driver->GetBufferSize = NULL;  /* REQUIRED! */
   driver->ResizeBuffers = _mesa_resize_framebuffer;
   driver->Error = NULL;

   driver->Finish = NULL;
   driver->Flush = NULL;

   /* framebuffer/image functions */
   driver->Clear = _swrast_Clear;
   driver->Accum = _mesa_accum;
   driver->RasterPos = _tnl_RasterPos;
   driver->DrawPixels = _swrast_DrawPixels;
   driver->ReadPixels = _mesa_readpixels;
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
   driver->CopyTexSubImage1D = _mesa_meta_CopyTexSubImage1D;
   driver->CopyTexSubImage2D = _mesa_meta_CopyTexSubImage2D;
   driver->CopyTexSubImage3D = _mesa_meta_CopyTexSubImage3D;
   driver->GenerateMipmap = _mesa_meta_GenerateMipmap;
   driver->TestProxyTexImage = _mesa_test_proxy_teximage;
   driver->BindTexture = NULL;
   driver->NewTextureObject = _mesa_new_texture_object;
   driver->DeleteTexture = _mesa_delete_texture_object;
   driver->NewTextureImage = _swrast_new_texture_image;
   driver->DeleteTextureImage = _swrast_delete_texture_image;
   driver->AllocTextureImageBuffer = _swrast_alloc_texture_image_buffer;
   driver->FreeTextureImageBuffer = _swrast_free_texture_image_buffer;
   driver->MapTextureImage = _swrast_map_teximage;
   driver->UnmapTextureImage = _swrast_unmap_teximage;

   /* Vertex/fragment programs */
   driver->BindProgram = NULL;
   driver->NewProgram = _mesa_new_program;
   driver->DeleteProgram = _mesa_delete_program;

   /* simple state commands */
   driver->AlphaFunc = NULL;
   driver->BlendColor = NULL;
   driver->BlendEquationSeparate = NULL;
   driver->BlendFuncSeparate = NULL;
   driver->ClearColor = NULL;
   driver->ClearDepth = NULL;
   driver->ClearStencil = NULL;
   driver->ClipPlane = NULL;
   driver->ColorMask = NULL;
   driver->ColorMaterial = NULL;
   driver->CullFace = NULL;
   driver->DrawBuffer = NULL;
   driver->FrontFace = NULL;
   driver->DepthFunc = NULL;
   driver->DepthMask = NULL;
   driver->DepthRange = NULL;
   driver->Enable = NULL;
   driver->Fogfv = NULL;
   driver->Hint = NULL;
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
   driver->Viewport = NULL;

   /* buffer objects */
   _mesa_init_buffer_object_functions(driver);

   driver->NewFramebuffer = _mesa_new_framebuffer;
   driver->NewRenderbuffer = _swrast_new_soft_renderbuffer;
   driver->MapRenderbuffer = _swrast_map_soft_renderbuffer;
   driver->UnmapRenderbuffer = _swrast_unmap_soft_renderbuffer;
   driver->RenderTexture = _swrast_render_texture;
   driver->FinishRenderTexture = _swrast_finish_render_texture;
   driver->FramebufferRenderbuffer = _mesa_framebuffer_renderbuffer;
   driver->ValidateFramebuffer = _mesa_validate_framebuffer;

   driver->BlitFramebuffer = _swrast_BlitFramebuffer;

   _mesa_init_texture_barrier_functions(driver);

   /* APPLE_vertex_array_object */
   driver->NewArrayObject = _mesa_new_array_object;
   driver->DeleteArrayObject = _mesa_delete_array_object;
   driver->BindArrayObject = NULL;

   _mesa_init_shader_object_functions(driver);

   /* T&L stuff */
   driver->CurrentExecPrimitive = 0;
   driver->CurrentSavePrimitive = 0;
   driver->NeedFlush = 0;
   driver->SaveNeedFlush = 0;

   driver->FlushVertices = NULL;
   driver->SaveFlushVertices = NULL;
   driver->PrepareExecBegin = NULL;
   driver->NotifySaveBegin = NULL;
   driver->LightingSpaceChange = NULL;

   /* display list */
   driver->NewList = NULL;
   driver->EndList = NULL;
   driver->BeginCallList = NULL;
   driver->EndCallList = NULL;

   /* GL_ARB_texture_storage */
   driver->AllocTextureStorage = _swrast_AllocTextureStorage;
}
