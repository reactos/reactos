<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdko" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdko.dll" allowwarnings="true">
	<importlibrary definition="kbdko.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdko.c</file>
	<file>kbdko.rc</file>
</module>
