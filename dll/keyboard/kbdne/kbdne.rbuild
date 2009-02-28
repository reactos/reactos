<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdne" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdne.dll">
	<importlibrary definition="kbdne.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdne.c</file>
	<file>kbdne.rc</file>
</module>
