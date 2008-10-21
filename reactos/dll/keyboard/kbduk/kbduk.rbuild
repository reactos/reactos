<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbduk" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbduk.dll" allowwarnings="true">
	<importlibrary definition="kbduk.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbduk.c</file>
	<file>kbduk.rc</file>
</module>
