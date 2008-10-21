<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdbur" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdbur.dll" allowwarnings="true">
	<importlibrary definition="kbdbur.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdbur.c</file>
	<file>kbdbur.rc</file>
</module>
