<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdth" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdth.dll" allowwarnings="true">
	<importlibrary definition="kbdth.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdth.c</file>
	<file>kbdth.rc</file>
</module>
