<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdtuq" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdtuq.dll" allowwarnings="true">
	<importlibrary definition="kbdtuq.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdtuq.c</file>
	<file>kbdtuq.rc</file>
</module>
