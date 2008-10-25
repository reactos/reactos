<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdlv" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdlv.dll" allowwarnings="true">
	<importlibrary definition="kbdlv.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdlv.c</file>
	<file>kbdlv.rc</file>
</module>
