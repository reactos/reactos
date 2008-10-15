<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="mesa32" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_MESA32}" installbase="system32" installname="mesa32.dll" allowwarnings="true">
	<importlibrary definition="src/drivers/windows/icd/mesa.def" />
	<linkerflag>-enable-stdcall-fixup</linkerflag>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>msvcrt</library>
	<library>user32</library>
	<define name="USE_EXTERNAL_DXTN_LIB" />
	<!-- The following is autogenrated by Makefile.ReactOS -->
	<library>gdi32</library>
	<define name="BUILD_GL32" />
	<define name="_OPENGL32_" />
	<define name="USE_EXTERNAL_DXTN_LIB=1" />
	<define name="USE_X86_ASM" />
	<define name="USE_MMX_ASM" />
	<define name="USE_SSE_ASM" />
	<define name="USE_3DNOW_ASM" />
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
	<directory name="src">
	<directory name="glapi">
		<file>glapi.c</file>
		<file>glthread.c</file>
	</directory>
	<directory name="math">
		<file>m_debug_clip.c</file>
		<file>m_debug_norm.c</file>
		<file>m_debug_xform.c</file>
		<file>m_eval.c</file>
		<file>m_matrix.c</file>
		<file>m_translate.c</file>
		<file>m_vector.c</file>
		<file>m_xform.c</file>
	</directory>
	<directory name="main">
		<file>accum.c</file>
		<file>api_arrayelt.c</file>
		<file>api_loopback.c</file>
		<file>api_noop.c</file>
		<file>api_validate.c</file>
		<file>arrayobj.c</file>
		<file>attrib.c</file>
		<file>blend.c</file>
		<file>bufferobj.c</file>
		<file>buffers.c</file>
		<file>clip.c</file>
		<file>colortab.c</file>
		<file>context.c</file>
		<file>convolve.c</file>
		<file>debug.c</file>
		<file>depth.c</file>
		<file>depthstencil.c</file>
		<file>dispatch.c</file>
		<file>dlist.c</file>
		<file>drawpix.c</file>
		<file>enable.c</file>
		<file>enums.c</file>
		<file>eval.c</file>
		<file>execmem.c</file>
		<file>extensions.c</file>
		<file>fbobject.c</file>
		<file>feedback.c</file>
		<file>fog.c</file>
		<file>framebuffer.c</file>
		<file>get.c</file>
		<file>getstring.c</file>
		<file>hash.c</file>
		<file>hint.c</file>
		<file>histogram.c</file>
		<file>image.c</file>
		<file>imports.c</file>
		<file>light.c</file>
		<file>lines.c</file>
		<file>matrix.c</file>
		<file>mipmap.c</file>
		<file>mm.c</file>
		<file>pixel.c</file>
		<file>points.c</file>
		<file>polygon.c</file>
		<file>queryobj.c</file>
		<file>rastpos.c</file>
		<file>rbadaptors.c</file>
		<file>renderbuffer.c</file>
		<file>texcompress.c</file>
		<file>texcompress_fxt1.c</file>
		<file>texcompress_s3tc.c</file>
		<file>texenvprogram.c</file>
		<file>texformat.c</file>
		<file>teximage.c</file>
		<file>texobj.c</file>
		<file>texrender.c</file>
		<file>texstate.c</file>
		<file>texstore.c</file>
		<file>varray.c</file>
	</directory>
	<directory name="shader">
		<file>arbprogparse.c</file>
		<file>arbprogram.c</file>
		<file>atifragshader.c</file>
		<file>nvfragparse.c</file>
		<file>nvprogram.c</file>
		<file>nvvertparse.c</file>
		<file>prog_debug.c</file>
		<file>prog_execute.c</file>
		<file>prog_instruction.c</file>
		<file>prog_parameter.c</file>
		<file>prog_print.c</file>
		<file>prog_statevars.c</file>
		<file>prog_uniform.c</file>
		<file>program.c</file>
		<file>programopt.c</file>
		<directory name="grammar">
			<file>grammar_mesa.c</file>
		</directory>
	</directory>
	<directory name="swrast">
		<file>s_aaline.c</file>
		<file>s_aatriangle.c</file>
		<file>s_accum.c</file>
		<file>s_alpha.c</file>
		<file>s_atifragshader.c</file>
		<file>s_bitmap.c</file>
		<file>s_blend.c</file>
		<file>s_blit.c</file>
		<file>s_buffers.c</file>
		<file>s_context.c</file>
		<file>s_copypix.c</file>
		<file>s_depth.c</file>
		<file>s_drawpix.c</file>
		<file>s_feedback.c</file>
		<file>s_fog.c</file>
		<file>s_fragprog.c</file>
		<file>s_imaging.c</file>
		<file>s_lines.c</file>
		<file>s_logic.c</file>
		<file>s_masking.c</file>
		<file>s_points.c</file>
		<file>s_readpix.c</file>
		<file>s_span.c</file>
		<file>s_stencil.c</file>
		<file>s_texcombine.c</file>
		<file>s_texfilter.c</file>
		<file>s_texstore.c</file>
		<file>s_triangle.c</file>
		<file>s_zoom.c</file>
	</directory>
	<directory name="main">
		<file>shaders.c</file>
	</directory>
	<directory name="shader">
		<file>shader_api.c</file>
		<directory name="slang">
			<file>slang_builtin.c</file>
			<file>slang_codegen.c</file>
			<file>slang_compile.c</file>
			<file>slang_compile_function.c</file>
			<file>slang_compile_operation.c</file>
			<file>slang_compile_struct.c</file>
			<file>slang_compile_variable.c</file>
			<file>slang_emit.c</file>
			<file>slang_ir.c</file>
			<file>slang_label.c</file>
			<file>slang_library_noise.c</file>
			<file>slang_link.c</file>
			<file>slang_log.c</file>
			<file>slang_mem.c</file>
			<file>slang_preprocess.c</file>
			<file>slang_print.c</file>
			<file>slang_simplify.c</file>
			<file>slang_storage.c</file>
			<file>slang_typeinfo.c</file>
			<file>slang_utility.c</file>
			<file>slang_vartable.c</file>
		</directory>
	</directory>
	<directory name="swrast_setup">
		<file>ss_context.c</file>
		<file>ss_triangle.c</file>
	</directory>
	<directory name="main">
		<file>state.c</file>
		<file>stencil.c</file>
	</directory>
	<directory name="tnl">
		<file>t_context.c</file>
		<file>t_draw.c</file>
		<file>t_pipeline.c</file>
		<file>t_vb_cull.c</file>
		<file>t_vb_fog.c</file>
		<file>t_vb_light.c</file>
		<file>t_vb_normals.c</file>
		<file>t_vb_points.c</file>
		<file>t_vb_program.c</file>
		<file>t_vb_render.c</file>
		<file>t_vb_texgen.c</file>
		<file>t_vb_texmat.c</file>
		<file>t_vb_vertex.c</file>
		<file>t_vertex.c</file>
		<file>t_vertex_generic.c</file>
		<file>t_vp_build.c</file>
		<file>t_vertex_sse.c</file>
	</directory>
	<directory name="vbo">
		<file>vbo_context.c</file>
		<file>vbo_exec.c</file>
		<file>vbo_exec_api.c</file>
		<file>vbo_exec_array.c</file>
		<file>vbo_exec_draw.c</file>
		<file>vbo_exec_eval.c</file>
		<file>vbo_rebase.c</file>
		<file>vbo_save.c</file>
		<file>vbo_save_api.c</file>
		<file>vbo_save_draw.c</file>
		<file>vbo_save_loopback.c</file>
		<file>vbo_split.c</file>
		<file>vbo_split_copy.c</file>
		<file>vbo_split_inplace.c</file>
	</directory>
	<directory name="main">
		<file>vtxfmt.c</file>
	</directory>
	<directory name="drivers">
		<directory name="common">
			<file>driverfuncs.c</file>
		</directory>
		<directory name="windows">
			<directory name="gdi">
				<file>wmesa.c</file>
				<file>wgl.c</file>
			</directory>
			<directory name="icd">
				<file>icd.c</file>
			</directory>
		</directory>
	</directory>
	<directory name="x86">
		<directory name="rtasm">
			<file>x86sse.c</file>
		</directory>
		<file>3dnow.c</file>
		<file>3dnow_normal.S</file>
		<file>3dnow_xform1.S</file>
		<file>3dnow_xform2.S</file>
		<file>3dnow_xform3.S</file>
		<file>3dnow_xform4.S</file>
		<file>common_x86.c</file>
		<file>common_x86_asm.S</file>
		<file>glapi_x86.S</file>
		<file>mmx_blend.S</file>
		<file>read_rgba_span_x86.S</file>
		<file>sse_normal.S</file>
		<file>sse_xform1.S</file>
		<file>sse_xform2.S</file>
		<file>sse_xform3.S</file>
		<file>sse_xform4.S</file>
		<file>x86.c</file>
		<file>x86_cliptest.S</file>
		<file>x86_xform2.S</file>
		<file>x86_xform3.S</file>
		<file>x86_xform4.S</file>
		<file>sse.c</file>
	</directory>
	<directory name="x86-64">
		<file>x86-64.c</file>
	</directory>
</directory>
</module>
