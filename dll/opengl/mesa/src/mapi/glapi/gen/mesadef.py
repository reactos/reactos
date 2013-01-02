#!/usr/bin/env python


# Mesa 3-D graphics library
# Version:  4.1
# 
# Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
# AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


# Generate the mesa.def file for Windows.
#
# Usage:
#    mesadef.py >mesa.def
#    Then copy to src/mesa/drivers/windows/gdi
#
# Dependencies:
#    The apispec file must be in the current directory.



import apiparser
import string


def PrintHead():
	print '; DO NOT EDIT - This file generated automatically by mesadef.py script'
	print 'DESCRIPTION \'Mesa (OpenGL work-alike) for Win32\''
	print 'VERSION 6.0'
	print ';'
	print '; Module definition file for Mesa (OPENGL32.DLL)'
	print ';'
	print '; Note: The OpenGL functions use the STDCALL'
	print '; function calling convention.  Microsoft\'s'
	print '; OPENGL32 uses this convention and so must the'
	print '; Mesa OPENGL32 so that the Mesa DLL can be used'
	print '; as a drop-in replacement.'
	print ';'
	print '; The linker exports STDCALL entry points with'
	print '; \'decorated\' names; e.g., _glBegin@0, where the'
	print '; trailing number is the number of bytes of '
	print '; parameter data pushed onto the stack.  The'
	print '; callee is responsible for popping this data'
	print '; off the stack, usually via a RETF n instruction.'
	print ';'
	print '; However, the Microsoft OPENGL32.DLL does not export'
	print '; the decorated names, even though the calling convention'
	print '; is STDCALL.  So, this module definition file is'
	print '; needed to force the Mesa OPENGL32.DLL to export the'
	print '; symbols in the same manner as the Microsoft DLL.'
	print '; Were it not for this problem, this file would not'
	print '; be needed (for the gl* functions) since the entry'
	print '; points are compiled with dllexport declspec.'
	print ';'
	print '; However, this file is still needed to export "internal"'
	print '; Mesa symbols for the benefit of the OSMESA32.DLL.'
	print ';'
	print 'EXPORTS'
	return
#enddef


def PrintTail():
	print ';'
	print '; WGL API'
	print '\twglChoosePixelFormat'
	print '\twglCopyContext'
	print '\twglCreateContext'
	print '\twglCreateLayerContext'
	print '\twglDeleteContext'
	print '\twglDescribeLayerPlane'
	print '\twglDescribePixelFormat'
	print '\twglGetCurrentContext'
	print '\twglGetCurrentDC'
	print '\twglGetExtensionsStringARB'
	print '\twglGetLayerPaletteEntries'
	print '\twglGetPixelFormat'
	print '\twglGetProcAddress'
	print '\twglMakeCurrent'
	print '\twglRealizeLayerPalette'
	print '\twglSetLayerPaletteEntries'
	print '\twglSetPixelFormat'
	print '\twglShareLists'
	print '\twglSwapBuffers'
	print '\twglSwapLayerBuffers'
	print '\twglUseFontBitmapsA'
	print '\twglUseFontBitmapsW'
	print '\twglUseFontOutlinesA'
	print '\twglUseFontOutlinesW'
	print ';'
	print '; Mesa internals - mostly for OSMESA'
	print '\t_ac_CreateContext'
	print '\t_ac_DestroyContext'
	print '\t_ac_InvalidateState'
	print '\t_glapi_get_context'
	print '\t_glapi_get_proc_address'
	print '\t_mesa_buffer_data'
	print '\t_mesa_buffer_map'
	print '\t_mesa_buffer_subdata'
	print '\t_mesa_choose_tex_format'
	print '\t_mesa_compressed_texture_size'
	print '\t_mesa_create_framebuffer'
	print '\t_mesa_create_visual'
	print '\t_mesa_delete_buffer_object'
	print '\t_mesa_delete_texture_object'
	print '\t_mesa_destroy_framebuffer'
	print '\t_mesa_destroy_visual'
	print '\t_mesa_enable_1_3_extensions'
	print '\t_mesa_enable_1_4_extensions'
	print '\t_mesa_enable_1_5_extensions'
	print '\t_mesa_enable_sw_extensions'
	print '\t_mesa_error'
	print '\t_mesa_free_context_data'
	print '\t_mesa_get_current_context'
	print '\t_mesa_init_default_imports'
	print '\t_mesa_initialize_context'
	print '\t_mesa_make_current'
	print '\t_mesa_new_buffer_object'
	print '\t_mesa_new_texture_object'
	print '\t_mesa_problem'
	print '\t_mesa_ResizeBuffersMESA'
	print '\t_mesa_store_compressed_teximage1d'
	print '\t_mesa_store_compressed_teximage2d'
	print '\t_mesa_store_compressed_teximage3d'
	print '\t_mesa_store_compressed_texsubimage1d'
	print '\t_mesa_store_compressed_texsubimage2d'
	print '\t_mesa_store_compressed_texsubimage3d'
	print '\t_mesa_store_teximage1d'
	print '\t_mesa_store_teximage2d'
	print '\t_mesa_store_teximage3d'
	print '\t_mesa_store_texsubimage1d'
	print '\t_mesa_store_texsubimage2d'
	print '\t_mesa_store_texsubimage3d'
	print '\t_mesa_test_proxy_teximage'
	print '\t_mesa_Viewport'
	print '\t_mesa_meta_CopyColorSubTable'
	print '\t_mesa_meta_CopyColorTable'
	print '\t_mesa_meta_CopyConvolutionFilter1D'
	print '\t_mesa_meta_CopyConvolutionFilter2D'
	print '\t_mesa_meta_CopyTexImage1D'
	print '\t_mesa_meta_CopyTexImage2D'
	print '\t_mesa_meta_CopyTexSubImage1D'
	print '\t_mesa_meta_CopyTexSubImage2D'
	print '\t_mesa_meta_CopyTexSubImage3D'
	print '\t_swrast_Accum'
	print '\t_swrast_alloc_buffers'
	print '\t_swrast_Bitmap'
	print '\t_swrast_CopyPixels'
	print '\t_swrast_DrawPixels'
	print '\t_swrast_GetDeviceDriverReference'
	print '\t_swrast_Clear'
	print '\t_swrast_choose_line'
	print '\t_swrast_choose_triangle'
	print '\t_swrast_CreateContext'
	print '\t_swrast_DestroyContext'
	print '\t_swrast_InvalidateState'
	print '\t_swrast_ReadPixels'
	print '\t_swrast_zbuffer_address'
	print '\t_swsetup_Wakeup'
	print '\t_swsetup_CreateContext'
	print '\t_swsetup_DestroyContext'
	print '\t_swsetup_InvalidateState'
	print '\t_tnl_CreateContext'
	print '\t_tnl_DestroyContext'
	print '\t_tnl_InvalidateState'
	print '\t_tnl_MakeCurrent'
	print '\t_tnl_run_pipeline'
#enddef


records = []

def FindOffset(funcName):
	for (name, alias, offset) in records:
		if name == funcName:
			return offset
		#endif
	#endfor
	return -1
#enddef


def EmitEntry(name, returnType, argTypeList, argNameList, alias, offset):
	if alias == '':
		dispatchName = name
	else:
		dispatchName = alias
	if offset < 0:
		offset = FindOffset(dispatchName)
	if offset >= 0 and string.find(name, "unused") == -1:
		print '\tgl%s' % (name)
		# save this info in case we need to look up an alias later
		records.append((name, dispatchName, offset))

#enddef


PrintHead()
apiparser.ProcessSpecFile("APIspec", EmitEntry)
PrintTail()
