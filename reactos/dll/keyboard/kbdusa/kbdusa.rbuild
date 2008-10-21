<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdusa" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdusa.dll" allowwarnings="true">
	<importlibrary definition="kbdusa.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdusa.c</file>
	<file>kbdusa.rc</file>
</module>
