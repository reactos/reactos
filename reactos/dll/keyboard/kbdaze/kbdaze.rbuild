<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdaze" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdaze.dll" allowwarnings="true">
	<importlibrary definition="kbdaze.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdaze.c</file>
	<file>kbdaze.rc</file>
</module>
