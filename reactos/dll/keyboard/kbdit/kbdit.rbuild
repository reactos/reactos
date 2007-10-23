<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdit" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdit.dll" allowwarnings="true">
	<importlibrary definition="kbdit.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdit.c</file>
	<file>kbdit.rc</file>
</module>
