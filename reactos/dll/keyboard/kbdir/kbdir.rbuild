<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdir" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdir.dll" allowwarnings="true">
	<importlibrary definition="kbdir.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdir.c</file>
	<file>kbdir.rc</file>
</module>
