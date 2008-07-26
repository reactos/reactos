<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="halamd64_generic" type="objectlibrary">
	<include base="halamd64_generic">../include</include>
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_NTHAL_" />
	<file>hal.c</file>
	<pch>../include/hal.h</pch>
</module>
