<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdus" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdus.dll" allowwarnings="true">
	<importlibrary definition="kbdus.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdus.c</file>
	<file>kbdus.rc</file>
</module>
