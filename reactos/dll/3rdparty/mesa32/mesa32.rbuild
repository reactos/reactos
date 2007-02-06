<!-- Autogeneratd by Makefile.ReactOS -->
<module name="mesa32" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_MESA32}" installbase="system32" installname="mesa32.dll" allowwarnings="true">
 	<importlibrary definition="src/drivers/windows/icd/mesa.def" />
 	<linkerflag>-Wl,--enable-stdcall-fixup</linkerflag>
 	<library>ntdll</library>
 	<library>kernel32</library>
 	<library>msvcrt</library>
 	<library>user32</library>
 	<define name="__USE_W32API" />
 	<define name="USE_EXTERNAL_DXTN_LIB" />
 	<!-- The following is autogenrated by Makefile.ReactOS -->
 	<library>gdi32</library>
 	<define name="BUILD_GL32" />
 	<define name="_OPENGL32_" />
 	<define name="USE_EXTERNAL_DXTN_LIB=1" />
 	<define name="USE_MGL_NAMESPACE" />
 	<define name="USE_X86_ASM" />
 	<include base="mesa32">include</include>
 	<include base="mesa32">src</include>
 	<include base="mesa32">src/main</include>
 	<include base="mesa32">src/glapi</include>
 	<include base="mesa32">src/math</include>
 	<include base="mesa32">src/tnl</include>
 	<include base="mesa32">src/shader</include>
 	<include base="mesa32">src/shader/grammar</include>
 	<include base="mesa32">src/shader/slang</include>
 	<include base="mesa32">src/shader/slang/OSDependent/Linux</include>
 	<include base="mesa32">src/shader/slang/OGLCompilersDLL</include>
 	<file>src/main/api_arrayelt.c</file>
 	<file>src/main/api_loopback.c</file>
 	<file>src/main/api_noop.c</file>
 	<file>src/main/api_validate.c</file>
 	<file>src/main/accum.c</file>
 	<file>src/main/attrib.c</file>
 	<file>src/main/blend.c</file>
 	<file>src/main/bufferobj.c</file>
 	<file>src/main/buffers.c</file>
 	<file>src/main/clip.c</file>
 	<file>src/main/colortab.c</file>
 	<file>src/main/context.c</file>
 	<file>src/main/convolve.c</file>
 	<file>src/main/debug.c</file>
 	<file>src/main/depth.c</file>
 	<file>src/main/dispatch.c</file>
 	<file>src/main/dlist.c</file>
 	<file>src/main/drawpix.c</file>
 	<file>src/main/enable.c</file>
 	<file>src/main/enums.c</file>
 	<file>src/main/eval.c</file>
 	<file>src/main/execmem.c</file>
 	<file>src/main/extensions.c</file>
 	<file>src/main/fbobject.c</file>
 	<file>src/main/feedback.c</file>
 	<file>src/main/fog.c</file>
 	<file>src/main/framebuffer.c</file>
 	<file>src/main/get.c</file>
 	<file>src/main/getstring.c</file>
 	<file>src/main/hash.c</file>
 	<file>src/main/hint.c</file>
 	<file>src/main/histogram.c</file>
 	<file>src/main/image.c</file>
 	<file>src/main/imports.c</file>
 	<file>src/main/light.c</file>
 	<file>src/main/lines.c</file>
 	<file>src/main/matrix.c</file>
 	<file>src/main/mm.c</file>
 	<file>src/main/occlude.c</file>
 	<file>src/main/pixel.c</file>
 	<file>src/main/points.c</file>
 	<file>src/main/polygon.c</file>
 	<file>src/main/rastpos.c</file>
 	<file>src/main/renderbuffer.c</file>
 	<file>src/main/state.c</file>
 	<file>src/main/stencil.c</file>
 	<file>src/main/texcompress.c</file>
 	<file>src/main/texcompress_s3tc.c</file>
 	<file>src/main/texcompress_fxt1.c</file>
 	<file>src/main/texenvprogram.c</file>
 	<file>src/main/texformat.c</file>
 	<file>src/main/teximage.c</file>
 	<file>src/main/texrender.c</file>
 	<file>src/main/texobj.c</file>
 	<file>src/main/texstate.c</file>
 	<file>src/main/texstore.c</file>
 	<file>src/main/varray.c</file>
 	<file>src/main/vtxfmt.c</file>
 	<file>src/glapi/glapi.c</file>
 	<file>src/glapi/glthread.c</file>
 	<file>src/math/m_debug_clip.c</file>
 	<file>src/math/m_debug_norm.c</file>
 	<file>src/math/m_debug_xform.c</file>
 	<file>src/math/m_eval.c</file>
 	<file>src/math/m_matrix.c</file>
 	<file>src/math/m_translate.c</file>
 	<file>src/math/m_vector.c</file>
 	<file>src/math/m_xform.c</file>
 	<file>src/array_cache/ac_context.c</file>
 	<file>src/array_cache/ac_import.c</file>
 	<file>src/tnl/t_array_api.c</file>
 	<file>src/tnl/t_array_import.c</file>
 	<file>src/tnl/t_context.c</file>
 	<file>src/tnl/t_pipeline.c</file>
 	<file>src/tnl/t_save_api.c</file>
 	<file>src/tnl/t_save_loopback.c</file>
 	<file>src/tnl/t_save_playback.c</file>
 	<file>src/tnl/t_vb_arbprogram.c</file>
 	<file>src/tnl/t_vb_arbprogram_sse.c</file>
 	<file>src/tnl/t_vb_program.c</file>
 	<file>src/tnl/t_vb_render.c</file>
 	<file>src/tnl/t_vb_texgen.c</file>
 	<file>src/tnl/t_vb_texmat.c</file>
 	<file>src/tnl/t_vb_vertex.c</file>
 	<file>src/tnl/t_vb_cull.c</file>
 	<file>src/tnl/t_vb_fog.c</file>
 	<file>src/tnl/t_vb_light.c</file>
 	<file>src/tnl/t_vb_normals.c</file>
 	<file>src/tnl/t_vb_points.c</file>
 	<file>src/tnl/t_vp_build.c</file>
 	<file>src/tnl/t_vertex.c</file>
 	<file>src/tnl/t_vertex_sse.c</file>
 	<file>src/tnl/t_vertex_generic.c</file>
 	<file>src/tnl/t_vtx_api.c</file>
 	<file>src/tnl/t_vtx_generic.c</file>
 	<file>src/tnl/t_vtx_x86.c</file>
 	<file>src/tnl/t_vtx_eval.c</file>
 	<file>src/tnl/t_vtx_exec.c</file>
 	<file>src/shader/arbfragparse.c</file>
 	<file>src/shader/arbprogparse.c</file>
 	<file>src/shader/arbprogram.c</file>
 	<file>src/shader/arbvertparse.c</file>
 	<file>src/shader/atifragshader.c</file>
 	<file>src/shader/grammar/grammar_mesa.c</file>
 	<file>src/shader/nvfragparse.c</file>
 	<file>src/shader/nvprogram.c</file>
 	<file>src/shader/nvvertexec.c</file>
 	<file>src/shader/nvvertparse.c</file>
 	<file>src/shader/program.c</file>
 	<file>src/shader/shaderobjects.c</file>
 	<file>src/shader/shaderobjects_3dlabs.c</file>
 	<file>src/swrast/s_fragprog_to_c.c</file>
 	<file>src/swrast/s_aaline.c</file>
 	<file>src/swrast/s_aatriangle.c</file>
 	<file>src/swrast/s_accum.c</file>
 	<file>src/swrast/s_alpha.c</file>
 	<file>src/swrast/s_atifragshader.c</file>
 	<file>src/swrast/s_bitmap.c</file>
 	<file>src/swrast/s_blend.c</file>
 	<file>src/swrast/s_buffers.c</file>
 	<file>src/swrast/s_copypix.c</file>
 	<file>src/swrast/s_context.c</file>
 	<file>src/swrast/s_depth.c</file>
 	<file>src/swrast/s_drawpix.c</file>
 	<file>src/swrast/s_feedback.c</file>
 	<file>src/swrast/s_fog.c</file>
 	<file>src/swrast/s_imaging.c</file>
 	<file>src/swrast/s_lines.c</file>
 	<file>src/swrast/s_logic.c</file>
 	<file>src/swrast/s_masking.c</file>
 	<file>src/swrast/s_nvfragprog.c</file>
 	<file>src/swrast/s_pixeltex.c</file>
 	<file>src/swrast/s_points.c</file>
 	<file>src/swrast/s_readpix.c</file>
 	<file>src/swrast/s_span.c</file>
 	<file>src/swrast/s_stencil.c</file>
 	<file>src/swrast/s_tcc.c</file>
 	<file>src/swrast/s_texture.c</file>
 	<file>src/swrast/s_texstore.c</file>
 	<file>src/swrast/s_triangle.c</file>
 	<file>src/swrast/s_zoom.c</file>
 	<file>src/swrast_setup/ss_context.c</file>
 	<file>src/swrast_setup/ss_triangle.c</file>
 	<file>src/x86/common_x86.c</file>
 	<file>src/x86/x86.c</file>
 	<file>src/x86/3dnow.c</file>
 	<file>src/x86/sse.c</file>
 	<file>src/x86/rtasm/x86sse.c</file>
 	<file>src/sparc/sparc.c</file>
 	<file>src/ppc/common_ppc.c</file>
 	<file>src/x86-64/x86-64.c</file>
 	<file>src/shader/slang/slang_assemble.c</file>
 	<file>src/shader/slang/slang_assemble_assignment.c</file>
 	<file>src/shader/slang/slang_assemble_conditional.c</file>
 	<file>src/shader/slang/slang_assemble_constructor.c</file>
 	<file>src/shader/slang/slang_assemble_typeinfo.c</file>
 	<file>src/shader/slang/slang_compile.c</file>
 	<file>src/shader/slang/slang_execute.c</file>
 	<file>src/shader/slang/slang_preprocess.c</file>
 	<file>src/shader/slang/slang_storage.c</file>
 	<file>src/shader/slang/slang_utility.c</file>
 	<file>src/x86/common_x86_asm.S</file>
 	<file>src/x86/x86_xform2.S</file>
 	<file>src/x86/x86_xform3.S</file>
 	<file>src/x86/x86_xform4.S</file>
 	<file>src/x86/x86_cliptest.S</file>
 	<file>src/x86/mmx_blend.S</file>
 	<file>src/x86/3dnow_xform1.S</file>
 	<file>src/x86/3dnow_xform2.S</file>
 	<file>src/x86/3dnow_xform3.S</file>
 	<file>src/x86/3dnow_xform4.S</file>
 	<file>src/x86/3dnow_normal.S</file>
 	<file>src/x86/sse_xform1.S</file>
 	<file>src/x86/sse_xform2.S</file>
 	<file>src/x86/sse_xform3.S</file>
 	<file>src/x86/sse_xform4.S</file>
 	<file>src/x86/sse_normal.S</file>
 	<file>src/x86/read_rgba_span_x86.S</file>
 	<file>src/tnl/t_vtx_x86_gcc.S</file>
 	<file>src/x86/glapi_x86.S</file>
 	<file>src/drivers/common/driverfuncs.c</file>
 	<file>src/drivers/windows/gdi/wmesa.c</file>
 	<file>src/drivers/windows/icd/icd.c</file>
</module>
