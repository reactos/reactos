<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdinguj" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdinguj.dll">
	<importlibrary definition="kbdinguj.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdinguj.c</file>
	<file>kbdinguj.rc</file>
</module>
