<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbddv" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbddv.dll" allowwarnings="true">
	<importlibrary definition="kbddv.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbddv.c</file>
	<file>kbddv.rc</file>
</module>
