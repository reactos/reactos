<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdinben" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdinben.dll">
	<importlibrary definition="kbdinben.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdinben.c</file>
	<file>kbdinben.rc</file>
</module>
