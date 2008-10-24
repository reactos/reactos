<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdda" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdda.dll" allowwarnings="true">
	<importlibrary definition="kbdda.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdda.c</file>
	<file>kbdda.rc</file>
</module>
