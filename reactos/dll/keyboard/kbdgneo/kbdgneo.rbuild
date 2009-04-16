<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdgneo" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdgneo.dll">
	<importlibrary definition="kbdgneo.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdgneo.c</file>
	<file>kbdgneo.rc</file>
</module>
