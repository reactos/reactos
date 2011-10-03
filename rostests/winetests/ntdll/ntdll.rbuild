<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ntdll_winetest" type="win32cui" installbase="bin" installname="ntdll_winetest.exe" allowwarnings="true">
	<include base="ntdll_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<library>ntdll</library>
	<library>user32</library>
	<file>atom.c</file>
	<file>change.c</file>
	<file>directory.c</file>
	<file>env.c</file>
	<file>error.c</file>
	<file>exception.c</file>
	<file>file.c</file>
	<file>generated.c</file>
	<file>info.c</file>
	<file>large_int.c</file>
	<file>om.c</file>
	<file>path.c</file>
	<file>pipe.c</file>
	<file>port.c</file>
	<file>reg.c</file>
	<file>rtlbitmap.c</file>
	<file>rtl.c</file>
	<file>rtlstr.c</file>
	<file>string.c</file>
	<file>time.c</file>
	<file>testlist.c</file>
</module>
