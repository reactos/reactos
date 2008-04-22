<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdfi" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdfi.dll" allowwarnings="true">
	<importlibrary definition="kbdfi.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdfi.c</file>
	<file>kbdfi.rc</file>
</module>
