<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdkaz" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdkaz.dll" allowwarnings="true">
	<importlibrary definition="kbdkaz.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdkaz.c</file>
	<file>kbdkaz.rc</file>
</module>
