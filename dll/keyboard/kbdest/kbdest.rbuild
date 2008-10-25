<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdest" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdest.dll" allowwarnings="true">
	<importlibrary definition="kbdest.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdest.c</file>
	<file>kbdest.rc</file>
</module>
