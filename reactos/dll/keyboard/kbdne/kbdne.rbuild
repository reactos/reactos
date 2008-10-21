<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdne" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdne.dll" allowwarnings="true">
	<importlibrary definition="kbdne.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdne.c</file>
	<file>kbdne.rc</file>
</module>
