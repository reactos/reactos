<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdfr" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdfr.dll">
	<importlibrary definition="kbdfr.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdfr.c</file>
	<file>kbdfr.rc</file>
</module>
