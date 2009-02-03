<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdal" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdal.dll">
	<importlibrary definition="kbdal.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdal.c</file>
	<file>kbdal.rc</file>
</module>
