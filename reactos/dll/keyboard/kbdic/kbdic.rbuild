<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdic" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdic.dll" allowwarnings="true">
	<importlibrary definition="kbdic.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdic.c</file>
	<file>kbdic.rc</file>
</module>
