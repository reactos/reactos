<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdazel" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdazel.dll">
	<importlibrary definition="kbdazel.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdazel.c</file>
	<file>kbdazel.rc</file>
</module>
