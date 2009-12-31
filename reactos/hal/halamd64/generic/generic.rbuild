<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<module name="hal_generic_amd64" type="objectlibrary">
		<include base="hal_generic_amd64">../include</include>
		<include base="ntoskrnl">include</include>
		<define name="_DISABLE_TIDENTS" />
		<define name="_NTHAL_" />
		<file>halinit.c</file>
		<file>misc.c</file>
		<file>mps.S</file>
		<file>usage.c</file>
		<file>pic.c</file>
		<pch>../include/hal.h</pch>
	</module>
</group>
