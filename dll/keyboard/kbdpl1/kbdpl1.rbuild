<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdpl1" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdpl1.dll" allowwarnings="true">
	<importlibrary definition="kbdpl1.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdpl1.c</file>
	<file>kbdpl1.rc</file>
</module>
