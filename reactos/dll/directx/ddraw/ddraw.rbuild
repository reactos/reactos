<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ddraw" type="win32dll" installbase="system32" installname="ddraw.dll" unicode="yes" crt="msvcrt">
	<importlibrary definition="ddraw.def" />
	<include base="ddraw">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>d3d8thk</library>
	<library>dxguid</library>
	<library>ole32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>pseh</library>

	<file>ddraw.rc</file>
	<file>main.c</file>
	<file>startup.c</file>
	<file>cleanup.c</file>

	<directory name="Ddraw">
		<file>ddraw_main.c</file>
		<file>ddraw_displaymode.c</file>
		<file>ddraw_setcooperativelevel.c</file>
		<file>GetCaps.c</file>
		<file>GetDeviceIdentifier.c</file>
		<file>ddraw_stubs.c</file>
		<file>callbacks_dd_hel.c</file>
	</directory>
	<directory name="Surface">
		<file>surface_stubs.c</file>
		<file>surface_main.c</file>
		<file>callbacks_surf_hel.c</file>
 		<file>createsurface.c</file>
	</directory>
	<directory name="Clipper">
		<file>clipper_stubs.c</file>
		<file>clipper_main.c</file>
	</directory>
	<directory name="Color">
		<file>color_stubs.c</file>
	</directory>
	<directory name="d3d">
		<file>DirectD3D_main.c</file>
	</directory>
	<directory name="Gamma">
		<file>gamma_stubs.c</file>
	</directory>
	<directory name="Kernel">
		<file>kernel_stubs.c</file>
	</directory>
	<directory name="Palette">
		<file>palette_stubs.c</file>
	</directory>
	<directory name="Videoport">
		<file>videoport_stubs.c</file>
	</directory>
	<directory name="Vtable">
		<file>DirectDraw7_Vtable.c</file>
		<file>DirectDraw4_Vtable.c</file>
		<file>DirectDraw2_Vtable.c</file>
		<file>DirectDraw_Vtable.c</file>
		<file>DirectDrawSurface7_Vtable.c</file>
		<file>DirectDrawSurface4_Vtable.c</file>
		<file>DirectDrawSurface3_Vtable.c</file>
		<file>DirectDrawSurface2_Vtable.c</file>
		<file>DirectDrawSurface_Vtable.c</file>
		<file>DirectD3D_Vtable.c</file>
		<file>DirectD3D2_Vtable.c</file>
		<file>DirectD3D3_Vtable.c</file>
		<file>DirectD3D7_Vtable.c</file>
	</directory>
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38054#c7 -->
	<compilerflag compilerset="gcc">-fno-unit-at-a-time</compilerflag>
</module>
