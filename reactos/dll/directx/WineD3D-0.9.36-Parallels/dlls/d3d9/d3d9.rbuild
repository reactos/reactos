<module name="d3d9" type="win32dll" entrypoint="0" installbase="system32" installname="d3d9.dll">  
	<importlibrary definition="d3d9.def" />
	<include base="d3d9">.</include>
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

      <file>basetexture.c</file>
      <file>cubetexture.c</file>
      <file>d3d9_main.c</file>
      <file>device.c</file>
      <file>directx.c</file>
      <file>indexbuffer.c</file>
      <file>pixelshader.c</file>
      <file>query.c</file>
      <file>resource.c</file>
      <file>stateblock.c</file>
      <file>surface.c</file>
      <file>swapchain.c</file>
      <file>texture.c</file>
      <file>vertexbuffer.c</file>
      <file>vertexdeclaration.c</file>
      <file>vertexshader.c</file>
      <file>volume.c</file>
      <file>volumetexture.c</file>
      <file>version.rc</file>
</module>
