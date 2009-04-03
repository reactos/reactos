<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdla" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdla.dll">
	<importlibrary definition="kbdla.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdla.c</file>
	<file>kbdla.rc</file>
</module>
