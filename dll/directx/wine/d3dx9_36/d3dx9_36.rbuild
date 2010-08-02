<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="d3dx9_36" type="win32dll" installbase="system32" installname="d3dx9_36.dll" unicode="yes">
	<importlibrary definition="d3dx9_36.spec" />
	<include base="d3dx9_36">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />

	<library>d3d9</library>
	<library>wine</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>msvcrt</library>
	<library>wined3d</library>

	<file>core.c</file>
	<file>d3dx9_36_main.c</file>
	<file>font.c</file>
	<file>math.c</file>
	<file>mesh.c</file>
	<file>shader.c</file>
	<file>sprite.c</file>
	<file>surface.c</file>
	<file>texture.c</file>
	<file>util.c</file>
	<file>version.rc</file>

	<dependency>wineheaders</dependency>
</module>
