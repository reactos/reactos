<module name="ddraw" type="win32dll" entrypoint="0"  installbase="system32" installname="ddraw.dll">  
	<importlibrary definition="ddraw.def" />
	<include base="ddraw">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<define name="WINE_NATIVEWIN32" />
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>winmm</library>
	<library>dxguid</library>

      <file>clipper.c</file>
      <file>ddraw.c</file>
      <file>ddraw_thunks.c</file>
      <file>device.c</file>
      <file>direct3d.c</file>
      <file>executebuffer.c</file>
      <file>gamma.c</file>
      <file>light.c</file>
      <file>main.c</file>
      <file>material.c</file>
      <file>palette.c</file>
      <file>parent.c</file>
      <file>regsvr.c</file>
      <file>surface.c</file>
      <file>surface_thunks.c </file>
      <file>texture.c</file>
      <file>utils.c</file>
      <file>vertexbuffer.c</file>
      <file>viewport.c</file>
      <file>version.rc</file>
</module>
