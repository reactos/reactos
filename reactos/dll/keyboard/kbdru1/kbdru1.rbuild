<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdru1" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdru1.dll" allowwarnings="true">
	<importlibrary definition="kbdru1.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdru1.c</file>
	<file>kbdru1.rc</file>
</module>
