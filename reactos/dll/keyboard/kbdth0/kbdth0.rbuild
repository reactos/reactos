<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdth0" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdth0.dll">
	<importlibrary definition="kbdth0.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdth0.c</file>
	<file>kbdth0.rc</file>
</module>
