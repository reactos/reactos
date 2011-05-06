<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="opengl32_winetest" type="win32cui" installbase="bin" installname="opengl32_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<define name="__ROS_LONG64__" />
	<include base="opengl32_winetest">.</include>
	<library>wine</library>
	<library>opengl32</library>
	<library>gdi32</library>
	<library>user32</library>
	<file>opengl.c</file>
	<file>testlist.c</file>
</module>
</group>
