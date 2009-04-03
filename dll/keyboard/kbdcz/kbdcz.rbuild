<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdcz" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdcz.dll">
	<importlibrary definition="kbdcz.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdcz.c</file>
	<file>kbdcz.rc</file>
</module>
