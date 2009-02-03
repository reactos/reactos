<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdth1" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdth1.dll">
	<importlibrary definition="kbdth1.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdth1.c</file>
	<file>kbdth1.rc</file>
</module>
