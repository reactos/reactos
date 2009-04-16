<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdinasa" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdinasa.dll">
	<importlibrary definition="kbdinasa.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdinasa.c</file>
	<file>kbdinasa.rc</file>
</module>
