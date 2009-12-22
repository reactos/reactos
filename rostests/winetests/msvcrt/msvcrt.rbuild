<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msvcrt_winetest" type="win32cui" installbase="bin" installname="msvcrt_winetest.exe" allowwarnings="true">
	<include base="msvcrt_winetest">.</include>
	<include base="msvcrt">include/reactos/wine/msvcrt</include>
	<define name="__ROS_LONG64__" />
	<define name="_CRT_NONSTDC_NO_DEPRECATE" />

	<!-- FIXME: workarounds until we have a proper oldnames library -->
	<define name="open">_open</define>
	<define name="close">_close</define>
	<define name="tell">_tell</define>
	<define name="unlink">_unlink</define>
	<define name="fdopen">_fdopen</define>
	<define name="lseek">_lseek</define>
	<define name="read">_read</define>
	<define name="write">_write</define>
	<define name="mkdir">_mkdir</define>
	<define name="rmdir">_rmdir</define>
	<define name="putenv">_putenv</define>

	<file>cpp.c</file>
	<file>data.c</file>
	<file>dir.c</file>
	<file>environ.c</file>
	<file>file.c</file>
	<file>headers.c</file>
	<file>heap.c</file>
	<file>printf.c</file>
	<file>scanf.c</file>
	<file>signal.c</file>
	<file>string.c</file>
	<file>testlist.c</file>
	<file>time.c</file>
</module>
</group>
