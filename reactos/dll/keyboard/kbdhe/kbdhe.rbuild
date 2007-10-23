<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdhe" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdhe.dll" allowwarnings="true">
	<importlibrary definition="kbdhe.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdhe.c</file>
	<file>kbdhe.rc</file>
</module>