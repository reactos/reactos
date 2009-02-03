<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdarmw" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdarmw.dll">
	<importlibrary definition="kbdarmw.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdarmw.c</file>
	<file>kbdarmw.rc</file>
</module>
