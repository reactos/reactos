<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdtuf" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdtuf.dll">
	<importlibrary definition="kbdtuf.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdtuf.c</file>
	<file>kbdtuf.rc</file>
</module>
