<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdurs" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdurs.dll" allowwarnings="true">
	<importlibrary definition="kbdurs.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdurs.c</file>
	<file>kbdurs.rc</file>
</module>
