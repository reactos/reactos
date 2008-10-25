<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdhu" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdhu.dll" allowwarnings="true">
	<importlibrary definition="kbdhu.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdhu.c</file>
	<file>kbdhu.rc</file>
</module>
