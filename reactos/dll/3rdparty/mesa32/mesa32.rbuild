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
		<directory name="array_cache">
			<file>ac_context.c</file>
			<file>ac_import.c</file>
		</directory>
		<directory name="drivers">
			<directory name="common">
				<file>driverfuncs.c</file>
			</directory>
			<directory name="windows">
				<directory name="gdi">
					<file>wmesa.c</file>
				</directory>
				<directory name="icd">
					<file>icd.c</file>
				</directory>
			</directory>
		</directory>
		<directory name="glapi">
			<file>glapi.c</file>
			<file>glthread.c</file>
		</directory>
		<directory name="main">
			<file>api_arrayelt.c</file>
			<file>api_loopback.c</file>
			<file>api_noop.c</file>
			<file>api_validate.c</file>
			<file>accum.c</file>
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
			<file>mm.c</file>
			<file>occlude.c</file>
			<file>pixel.c</file>
			<file>points.c</file>
			<file>polygon.c</file>
			<file>rastpos.c</file>
			<file>renderbuffer.c</file>
			<file>state.c</file>
			<file>stencil.c</file>
			<file>texcompress.c</file>
			<file>texcompress_s3tc.c</file>
			<file>texcompress_fxt1.c</file>
			<file>texenvprogram.c</file>
			<file>texformat.c</file>
			<file>teximage.c</file>
			<file>texrender.c</file>
			<file>texobj.c</file>
			<file>texstate.c</file>
			<file>texstore.c</file>
			<file>varray.c</file>
			<file>vtxfmt.c</file>
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
		<directory name="ppc">
			<file>common_ppc.c</file>
		</directory>
		<directory name="shader">
			<directory name="grammar">
				<file>grammar_mesa.c</file>
			</directory>
			<directory name="slang">
				<file>slang_assemble.c</file>
				<file>slang_assemble_assignment.c</file>
				<file>slang_assemble_conditional.c</file>
				<file>slang_assemble_constructor.c</file>
				<file>slang_assemble_typeinfo.c</file>
				<file>slang_compile.c</file>
				<file>slang_execute.c</file>
				<file>slang_preprocess.c</file>
				<file>slang_storage.c</file>
				<file>slang_utility.c</file>
			</directory>
			<file>arbfragparse.c</file>
			<file>arbprogparse.c</file>
			<file>arbprogram.c</file>
			<file>arbvertparse.c</file>
			<file>atifragshader.c</file>
			<file>nvfragparse.c</file>
			<file>nvprogram.c</file>
			<file>nvvertexec.c</file>
			<file>nvvertparse.c</file>
			<file>program.c</file>
			<file>shaderobjects.c</file>
			<file>shaderobjects_3dlabs.c</file>
		</directory>
		<directory name="sparc">
			<file>sparc.c</file>
		</directory>
		<directory name="swrast">
			<file>s_fragprog_to_c.c</file>
			<file>s_aaline.c</file>
			<file>s_aatriangle.c</file>
			<file>s_accum.c</file>
			<file>s_alpha.c</file>
			<file>s_atifragshader.c</file>
			<file>s_bitmap.c</file>
			<file>s_blend.c</file>
			<file>s_buffers.c</file>
			<file>s_copypix.c</file>
			<file>s_context.c</file>
			<file>s_depth.c</file>
			<file>s_drawpix.c</file>
			<file>s_feedback.c</file>
			<file>s_fog.c</file>
			<file>s_imaging.c</file>
			<file>s_lines.c</file>
			<file>s_logic.c</file>
			<file>s_masking.c</file>
			<file>s_nvfragprog.c</file>
			<file>s_pixeltex.c</file>
			<file>s_points.c</file>
			<file>s_readpix.c</file>
			<file>s_span.c</file>
			<file>s_stencil.c</file>
			<file>s_tcc.c</file>
			<file>s_texture.c</file>
			<file>s_texstore.c</file>
			<file>s_triangle.c</file>
			<file>s_zoom.c</file>
		</directory>
		<directory name="swrast_setup">
			<file>ss_context.c</file>
			<file>ss_triangle.c</file>
		</directory>
		<directory name="tnl">
			<file>t_array_api.c</file>
			<file>t_array_import.c</file>
			<file>t_context.c</file>
			<file>t_pipeline.c</file>
			<file>t_save_api.c</file>
			<file>t_save_loopback.c</file>
			<file>t_save_playback.c</file>
			<file>t_vb_arbprogram.c</file>
			<file>t_vb_arbprogram_sse.c</file>
			<file>t_vb_program.c</file>
			<file>t_vb_render.c</file>
			<file>t_vb_texgen.c</file>
			<file>t_vb_texmat.c</file>
			<file>t_vb_vertex.c</file>
			<file>t_vb_cull.c</file>
			<file>t_vb_fog.c</file>
			<file>t_vb_light.c</file>
			<file>t_vb_normals.c</file>
			<file>t_vb_points.c</file>
			<file>t_vp_build.c</file>
			<file>t_vertex.c</file>
			<file>t_vertex_sse.c</file>
			<file>t_vertex_generic.c</file>
			<file>t_vtx_api.c</file>
			<file>t_vtx_generic.c</file>
			<file>t_vtx_eval.c</file>
			<file>t_vtx_exec.c</file>
			<file>t_vtx_x86.c</file>
			<file>t_vtx_x86_gcc.S</file>
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
