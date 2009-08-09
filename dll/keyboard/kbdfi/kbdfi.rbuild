<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdfi" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdfi.dll">
	<importlibrary definition="kbdfi.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdfi.c</file>
	<file>kbdfi.rc</file>
</module>
