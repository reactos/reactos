<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdvntc" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdvntc.dll">
	<importlibrary definition="kbdvntc.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdvntc.c</file>
	<file>kbdvntc.rc</file>
</module>
