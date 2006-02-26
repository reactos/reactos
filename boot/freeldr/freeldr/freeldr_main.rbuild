<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="freeldr_main" type="objectlibrary">
	<include base="freeldr_main">include</include>
	<include base="ntoskrnl">include</include>
	<define name="__USE_W32API" />
<!--	
	<define name="DEBUG" />
-->	
	<compilerflag>-ffreestanding</compilerflag>
	<compilerflag>-fno-builtin</compilerflag>
	<compilerflag>-fno-inline</compilerflag>
	<compilerflag>-fno-zero-initialized-in-bss</compilerflag>
	<compilerflag>-Os</compilerflag>
	<file>bootmgr.c</file>
	<file>drivemap.c</file>
	<file>miscboot.c</file>
	<file>options.c</file>
	<file>linuxboot.c</file>
	<file>oslist.c</file>
	<file>custom.c</file>
</module>
</rbuild>
