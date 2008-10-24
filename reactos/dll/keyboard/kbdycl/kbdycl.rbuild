<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdycl" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdycl.dll" allowwarnings="true">
	<importlibrary definition="kbdycl.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdycl.c</file>
	<file>kbdycl.rc</file>
</module>
