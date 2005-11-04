/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "imports.h"
#include "buffers.h"
#include "context.h"
#include "program.h"
#include "texcompress.h"
#include "texformat.h"
#include "teximage.h"
#include "texobj.h"
#include "texstore.h"
#include "bufferobj.h"

#include "driverfuncs.h"
#include "swrast/swrast.h"



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
   driver->ResizeBuffers = _swrast_alloc_buffers;
   driver->Error = NULL;

   driver->Finish = NULL;
   driver->Flush = NULL;

   /* framebuffer/image functions */
   driver->Clear = _swrast_Clear;
   driver->Accum = _swrast_Accum;
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
   driver->CopyTexImage1D = _swrast_copy_teximage1d;
   driver->CopyTexImage2D = _swrast_copy_teximage2d;
   driver->CopyTexSubImage1D = _swrast_copy_texsubimage1d;
   driver->CopyTexSubImage2D = _swrast_copy_texsubimage2d;
   driver->CopyTexSubImage3D = _swrast_copy_texsubimage3d;
   driver->TestProxyTexImage = _mesa_test_proxy_teximage;
   driver->CompressedTexImage1D = _mesa_store_compressed_teximage1d;
   driver->CompressedTexImage2D = _mesa_store_compressed_teximage2d;
   driver->CompressedTexImage3D = _mesa_store_compressed_teximage3d;
   driver->CompressedTexSubImage1D = _mesa_store_compressed_texsubimage1d;
   driver->CompressedTexSubImage2D = _mesa_store_compressed_texsubimage2d;
   driver->CompressedTexSubImage3D = _mesa_store_compressed_texsubimage3d;
   driver->CompressedTextureSize = _mesa_compressed_texture_size;
   driver->BindTexture = NULL;
   driver->NewTextureObject = _mesa_new_texture_object;
   driver->DeleteTexture = _mesa_delete_texture_object;
   driver->NewTextureImage = _mesa_new_texture_image;
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
   driver->DrawBuffer = _swrast_DrawBuffer;
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
   driver->StencilFunc = NULL;
   driver->StencilMask = NULL;
   driver->StencilOp = NULL;
   driver->ActiveStencilFace = NULL;
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
   driver->UnmapBuffer = NULL;
#endif

   /* T&L stuff */
   driver->NeedValidate = GL_FALSE;
   driver->ValidateTnlModule = NULL;
   driver->CurrentExecPrimitive = 0;
   driver->CurrentSavePrimitive = 0;
   driver->NeedFlush = 0;
   driver->SaveNeedFlush = 0;

   driver->FlushVertices = NULL;
   driver->SaveFlushVertices = NULL;
   driver->NotifySaveBegin = NULL;
   driver->LightingSpaceChange = NULL;
   driver->MakeCurrent = NULL;

   /* display list */
   driver->NewList = NULL;
   driver->EndList = NULL;
   driver->BeginCallList = NULL;
   driver->EndCallList = NULL;
}
