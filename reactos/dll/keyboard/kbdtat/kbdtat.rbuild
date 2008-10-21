<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdtat" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdtat.dll" allowwarnings="true">
	<importlibrary definition="kbdtat.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdtat.c</file>
	<file>kbdtat.rc</file>
</module>
