<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdno" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdno.dll" allowwarnings="true">
	<importlibrary definition="kbdno.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdno.c</file>
	<file>kbdno.rc</file>
</module>
