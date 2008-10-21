<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdcan" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdcan.dll" allowwarnings="true">
	<importlibrary definition="kbdcan.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdcan.c</file>
	<file>kbdcan.rc</file>
</module>
