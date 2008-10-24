<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdsk1" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdsk1.dll" allowwarnings="true">
	<importlibrary definition="kbdsk1.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdsk1.c</file>
	<file>kbdsk1.rc</file>
</module>
