<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdja" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdja.dll" allowwarnings="true">
	<importlibrary definition="kbdja.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdja.c</file>
	<file>kbdja.rc</file>
</module>
