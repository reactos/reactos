<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdth2" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdth2.dll">
	<importlibrary definition="kbdth2.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdth2.c</file>
	<file>kbdth2.rc</file>
</module>
