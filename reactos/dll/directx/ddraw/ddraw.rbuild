<module name="ddraw" type="win32dll" entrypoint="0" installbase="system32" installname="ddraw.dll" allowwarnings ="true">
  <importlibrary definition="ddraw.def" />
	<include base="ddraw">.</include>	
	<define name="UNICODE" />
	<define name="__USE_W32API" />
	<define name="WINVER">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>

	<library>ntdll</library>          
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>d3d8thk</library>
	<library>dxguid</library>
	<library>ole32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>msvcrt</library>

	<file>ddraw.rc</file>
	<file>main.c</file>
	<file>startup.c</file>
	<file>cleanup.c</file>
	<file>createsurface.c</file>

	<file>iface_clipper.c</file>
	<file>iface_color.c</file>
	<file>iface_gamma.c</file>
	<file>iface_palette.c</file>
	<file>iface_videoport.c</file>
	<file>iface_kernel.c</file>

      <file>callbacks_hel.c</file>
      <file>callbacks_surf_hel.c</file>

	<directory name="main">
		<file>ddraw_main.c</file>
		<file>surface_main.c</file>				
	</directory>	

	
</module>
