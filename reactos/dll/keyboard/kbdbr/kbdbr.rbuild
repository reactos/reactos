<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdbr" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdbr.dll" allowwarnings="true">
	<importlibrary definition="kbdbr.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdbr.c</file>
	<file>kbdbr.rc</file>
</module>
