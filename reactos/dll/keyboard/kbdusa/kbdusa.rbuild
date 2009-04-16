<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdusa" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdusa.dll">
	<importlibrary definition="kbdusa.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdusa.c</file>
	<file>kbdusa.rc</file>
</module>
