<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdgeo" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdgeo.dll" allowwarnings="true">
	<importlibrary definition="kbdgeo.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdgeo.c</file>
	<file>kbdgeo.rc</file>
</module>
