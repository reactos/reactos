<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdgerg" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdgerg.dll" allowwarnings="true">
	<importlibrary definition="kbdgerg.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdgerg.c</file>
	<file>kbdgerg.rc</file>
</module>
