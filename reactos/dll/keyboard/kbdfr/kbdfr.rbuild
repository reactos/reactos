<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdfr" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdfr.dll" allowwarnings="true">
	<importlibrary definition="kbdfr.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdfr.c</file>
	<file>kbdfr.rc</file>
</module>
