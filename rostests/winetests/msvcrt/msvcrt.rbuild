<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msvcrt_winetest" type="win32cui" installbase="bin" installname="msvcrt_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="msvcrt_winetest">.</include>
	<include base="msvcrt">include/reactos/wine/msvcrt</include>
	<define name="__USE_W32API" />
    <define name="__ROS_LONG64__" />
	<library>kernel32</library>
	<library>msvcrt</library>
	<file>cpp.c</file>
	<file>data.c</file>
	<file>dir.c</file>
	<file>environ.c</file>
	<file>file.c</file>
	<file>headers.c</file>
	<file>heap.c</file>
	<file>printf.c</file>
	<file>scanf.c</file>
	<file>string.c</file>
	<file>testlist.c</file>
	<file>time.c</file>
</module>
</group>
