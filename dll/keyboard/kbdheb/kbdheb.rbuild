<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdheb" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdheb.dll">
	<importlibrary definition="kbdheb.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdheb.c</file>
	<file>kbdheb.rc</file>
</module>
