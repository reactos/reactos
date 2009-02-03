<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdsw" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdsw.dll">
	<importlibrary definition="kbdsw.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdsw.c</file>
	<file>kbdsw.rc</file>
</module>
