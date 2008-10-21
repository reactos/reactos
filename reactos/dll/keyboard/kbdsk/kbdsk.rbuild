<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdsk" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdsk.dll" allowwarnings="true">
	<importlibrary definition="kbdsk.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdsk.c</file>
	<file>kbdsk.rc</file>
</module>
