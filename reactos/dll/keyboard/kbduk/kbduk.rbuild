<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbduk" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbduk.dll">
	<importlibrary definition="kbduk.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbduk.c</file>
	<file>kbduk.rc</file>
</module>
