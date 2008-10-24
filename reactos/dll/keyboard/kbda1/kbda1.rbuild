<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbda1" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbda1.dll" allowwarnings="true">
	<importlibrary definition="kbda1.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbda1.c</file>
	<file>kbda1.rc</file>
</module>
