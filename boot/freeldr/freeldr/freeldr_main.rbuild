<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="freeldr_main" type="objectlibrary">
	<include base="freeldr_main">include</include>
	<include base="ntoskrnl">include</include>
	<define name="_NTHAL_" />
	<compilerflag>-fno-inline</compilerflag>
	<compilerflag>-fno-zero-initialized-in-bss</compilerflag>
	<file>bootmgr.c</file>
	<file>drivemap.c</file>
	<file>miscboot.c</file>
	<file>options.c</file>
	<file>linuxboot.c</file>
	<file>oslist.c</file>
	<file>custom.c</file>
</module>
