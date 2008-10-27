<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdsg" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdsg.dll" allowwarnings="true">
	<importlibrary definition="kbdsg.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdsg.c</file>
	<file>kbdsg.rc</file>
</module>
