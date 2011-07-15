<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="d3dx9_42" type="win32dll" installbase="system32" installname="d3dx9_42.dll" unicode="yes">
	<importlibrary definition="d3dx9_42.spec" />
	<include base="d3dx9_42">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />

	<library>d3d9</library>
	<library>wine</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>wined3d</library>

	<file>d3dx9_42_main.c</file>
	<file>version.rc</file>

	<dependency>wineheaders</dependency>
</module>
