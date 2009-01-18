<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="wined3d" type="win32dll" installbase="system32" installname="wined3d.dll" allowwarnings ="true" crt="msvcrt">
	<importlibrary definition="wined3d.def" />
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="wined3d">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="USE_WIN32_OPENGL" />

	<library>wine</library>
	<library>user32</library>
	<library>opengl32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>uuid</library>

	<file>ati_fragment_shader.c</file>
	<file>arb_program_shader.c</file>
	<file>baseshader.c</file>
	<file>basetexture.c</file>
	<file>clipper.c</file>
	<file>context.c</file>
	<file>cubetexture.c</file>
	<file>device.c</file>
	<file>directx.c</file>
	<file>drawprim.c</file>
	<file>gl_compat.c</file>
	<file>glsl_shader.c</file>
	<file>indexbuffer.c</file>
	<file>nvidia_texture_shader.c</file>
	<file>palette.c</file>
	<file>pixelshader.c</file>
	<file>query.c</file>
	<file>resource.c</file>
	<file>state.c</file>
	<file>stateblock.c</file>
	<file>surface_base.c</file>
	<file>surface.c</file>
	<file>surface_gdi.c </file>
	<file>swapchain.c</file>
	<file>swapchain_base.c</file>
	<file>swapchain_gdi.c</file>
	<file>texture.c</file>
	<file>utils.c</file>
	<file>vertexbuffer.c</file>
	<file>vertexdeclaration.c</file>
	<file>vertexshader.c</file>
	<file>volume.c</file>
	<file>volumetexture.c</file>
	<file>wined3d_main.c</file>
	<file>version.rc</file>

	<dependency>wineheaders</dependency>
</module>
