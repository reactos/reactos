<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="freeldr_main" type="objectlibrary">
	<include base="freeldr_main">include</include>
	<include base="ntoskrnl">include</include>
	<define name="_NTHAL_" />
	<group compilerset="gcc">
		<compilerflag>-fno-inline</compilerflag>
		<compilerflag>-fno-zero-initialized-in-bss</compilerflag>
	</group>
	<file>bootmgr.c</file>
</module>
