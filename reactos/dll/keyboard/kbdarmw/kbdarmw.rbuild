<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdarmw" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdarmw.dll" allowwarnings="true">
	<importlibrary definition="kbdarmw.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdarmw.c</file>
	<file>kbdarmw.rc</file>
</module>
