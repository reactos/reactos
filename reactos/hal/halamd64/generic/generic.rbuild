<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<module name="hal_generic" type="objectlibrary">
		<include base="hal_generic">../include</include>
		<include base="ntoskrnl">include</include>
		<define name="_DISABLE_TIDENTS" />
		<define name="_NTHAL_" />
		<define name="_X86BIOS_" />
		<file>halinit.c</file>
		<file>irq.S</file>
		<file>misc.c</file>
		<file>mps.S</file>
		<file>systimer.S</file>
		<file>usage.c</file>
		<file>pic.c</file>
		<file>x86bios.c</file>
		<pch>../include/hal.h</pch>
	</module>
</group>
