<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="halarm_generic" type="objectlibrary">
	<include base="halarm_generic">../include</include>
	<include base="ntoskrnl">include</include>
	<define name="_NTHAL_" />
	<file>hal.c</file>
	<pch>../include/hal.h</pch>
</module>
