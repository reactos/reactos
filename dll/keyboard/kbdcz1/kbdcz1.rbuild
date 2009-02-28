<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdcz1" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdcz1.dll">
	<importlibrary definition="kbdcz1.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdcz1.c</file>
	<file>kbdcz1.rc</file>
</module>
