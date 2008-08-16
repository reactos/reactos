<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdmac" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdmac.dll" allowwarnings="true">
	<importlibrary definition="kbdmac.spec.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdmac.c</file>
	<file>kbdmac.rc</file>
	<file>kbdmac.spec</file>
</module>
